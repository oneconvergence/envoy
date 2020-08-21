#include <experimental/filesystem>
#include <future>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include <thread>

// keep this macro definition here only before any external/local includes
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "yaml-cpp/yaml.h"

#include "ads_impl.hpp"

namespace fs = std::experimental::filesystem;

bool server_done = false;
std::shared_ptr<spdlog::logger> logger = NULL;
void* ads_stream_ptr = NULL;
bool is_delta_stream = false;
StreamContext* ads_stream_ctx_ptr = NULL;
std::map<std::string, Resource> ads_resources;
std::mutex m1, m2;
std::condition_variable cv;
bool secrets_acked;
#ifdef HAS_SECRETS
bool listener_acked;
#endif
int total_secrets; // update this only with a lock on object m2

typedef struct UpdateInfo_ {
  ResourceType type;
  std::vector<std::string> listener;
  std::vector<std::string> l_config;
  std::vector<int>::size_type l_config_size;
  std::vector<int>::size_type l_config_index;
  int s_updates;
} UpdateInfo;

#ifdef HAS_SECRETS
void HandleDynamicUpdate(const std::string& operation,
                         const std::vector<std::string>& resource_names,
                         bool has_secrets);
#else
void HandleDynamicUpdate(const std::string& operation,
                         const std::vector<std::string>& resource_names);
#endif

class Updater {
 public:
  Updater(std::shared_future<void> f, std::chrono::seconds interval,
          UpdateInfo& update_info)
      : update_info(update_info), interval(interval), done(f) {}
  std::future<void> run() {
    return std::async(std::launch::async, &Updater::UpdateWrapper, this);
  }
 private:
  void UpdateWrapper() {
    std::future_status status;
    do {
      status = done.wait_for(interval);
      if (std::future_status::timeout == status)
        Update();
    } while (status != std::future_status::ready);
  }
  void Update();
  UpdateInfo update_info;
  const std::chrono::seconds interval;
  std::shared_future<void> done;
};

void Updater::Update() {
  switch (update_info.type) {
    case LISTENER: {
      if (!update_info.l_config_size)
        return;

      auto l_config = update_info.l_config[update_info.l_config_index];
      update_info.l_config_index++;
      if (update_info.l_config_index == update_info.l_config_size)
        update_info.l_config_index = 0;

      // mutex under lock gaurd
      {
        const std::lock_guard<std::mutex> lock(m2);

        // Update the listener
        secrets_acked = false;
        fs::copy("xds/lds/ilds/listener.yaml." + l_config,
                 "xds/lds/listener1.yaml",
                 fs::copy_options::overwrite_existing);
        std::cout << "Started updating the listener with "
                  << l_config << " secrets.." << std::endl;
#ifdef HAS_SECRETS
        HandleDynamicUpdate("update", update_info.listener, true);
#else
        HandleDynamicUpdate("update", update_info.listener);
#endif
        auto start = std::chrono::high_resolution_clock::now();
        // Wait until secrets acked
        {
          std::unique_lock<std::mutex> lk(m1);
          cv.wait(lk, []{return secrets_acked;});
        }
        std::chrono::duration<double> diff = 
          std::chrono::high_resolution_clock::now()-start;
        std::cout << "Listener moved to active state after "
                  << diff.count() << " sec." << std::endl;

        total_secrets = std::stoi(l_config);
      }
      break;
    }
    case RC: {break;}
    case SECRET: {
      auto num_secrets_to_update = update_info.s_updates;

      // mutex under lock gaurd
      {
        const std::lock_guard<std::mutex> lock(m2);

        std::vector<std::string> secrets_to_update;
        srand((unsigned) time(NULL));
        for (int i = 1; i <= num_secrets_to_update; i++)
          secrets_to_update.push_back(
            "service" + std::to_string(rand() % 8999 + 1000)
            + ".ocenvoy.com");

        // Update the secrets
        secrets_acked = false;
        std::cout << "Started updating " << num_secrets_to_update
                  << " random secrets.." << std::endl;
#ifdef HAS_SECRETS
        HandleDynamicUpdate("update", secrets_to_update, true);
#else
        HandleDynamicUpdate("update", secrets_to_update);
#endif
        auto start = std::chrono::high_resolution_clock::now();
        // Wait until secrets acked
        {
          std::unique_lock<std::mutex> lk(m1);
          cv.wait(lk, []{return secrets_acked;});
        }
        std::chrono::duration<double> diff =
          std::chrono::high_resolution_clock::now()-start;
        std::cout << "All secret updates acked after " << diff.count()
                  << " sec." << std::endl;
      }
      break;
    }
    case VH: {break;}
  }
}

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  AggregatedDiscoveryServiceImpl adsService;
  int selected_port = 0;

  grpc::ServerBuilder builder;

  grpc::SslServerCredentialsOptions sslOpts{};
  sslOpts.client_certificate_request =
    GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
  sslOpts.pem_key_cert_pairs.push_back(
    grpc::SslServerCredentialsOptions::PemKeyCertPair{
      ReadFile("server.key"), ReadFile("server.crt")});
  sslOpts.pem_root_certs = ReadFile("client.crt");

  auto creds = grpc::SslServerCredentials(sslOpts);

  builder.AddListeningPort(server_address, creds, &selected_port);
  builder.RegisterService(&adsService);
  builder.AddChannelArgument(GRPC_ARG_MAX_CONCURRENT_STREAMS, 10);
  builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  if (0 == selected_port) {
    std::cout << "Can not use port 50051! Press Enter to exit."
              << std::endl;
    server_done = true;
    return;
  }
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

/* Utility function to invoke stream type based send response calls
 * on a resource update. */
#ifdef HAS_SECRETS
void HandleDynamicUpdate(const std::string& operation,
                         const std::vector<std::string>& resource_names,
                         bool has_secrets) {
#else
void HandleDynamicUpdate(const std::string& operation,
                         const std::vector<std::string>& resource_names) {
#endif
  Resource* resource = NULL;

  if (!ads_stream_ctx_ptr || !ads_stream_ptr) {
    std::cout << "No active stream!" << std::endl;
    return;
  }

  // FIXME
  if (!is_delta_stream) {
    std::cout << "Updates over SotW stream are not supported!" << std::endl;
    return;
  }

  if (!is_delta_stream && "remove" == operation) {
    std::cout << "Removal of resource over the SotW stream not supported."
              << std::endl;
    return;
  }

  if ("add_listener" == operation) {
    if (1 < resource_names.size()) {
      std::cout << "Only one resource at a time is allowed to add."
                << std::endl;
      return;
    }

    std::string resource_name = resource_names[0];

    if (is_resource_present(resource_name)) {
      std::cout << "Listener with name \"" << resource_name
                << "\" already added. You can update it instead." << std::endl;
      return;
    }

    std::set<std::string> delta_resources;
    std::set<std::string> removed_resources;

    if (ads_stream_ctx_ptr->inc_resource_versions[LISTENER]
                          ->find(resource_name) ==
        ads_stream_ctx_ptr->inc_resource_versions[LISTENER]
                          ->end())
      ads_stream_ctx_ptr->inc_resource_versions[LISTENER]->insert(
        std::pair<std::string, std::string>(resource_name, ""));
    add_resource(resource_name, V3_LISTENER);
    delta_resources.insert(resource_name);

#ifdef HAS_SECRETS
    secrets_acked = false;
#endif
    SendADSResponseIncremental(V3_LISTENER,
                               &delta_resources, &removed_resources);
#ifdef HAS_SECRETS
    auto start = std::chrono::high_resolution_clock::now();
    // Wait until secrets acked
    {
      std::unique_lock<std::mutex> lk(m1);
      cv.wait(lk, []{return secrets_acked;});
    }
    std::chrono::duration<double> diff =
      std::chrono::high_resolution_clock::now()-start;
    std::cout << "Listener update - " << diff.count()
              << " sec." << std::endl;
#endif
    return;
  }

  /* commenting this code as Envoy is discarding dynamic addition of VH */
  /*
  if ("add_vh" == operation) {
    if (1 < resource_names.size()) {
      std::cout << "Only one resource at a time is allowed to add."
                << std::endl;
      return;
    }

    std::string resource_name = resource_names[0];

    if (is_resource_present(resource_name)) {
      std::cout << "Virtual host with name \"" << resource_name
                << "\" already added. You can update it instead."
                << std::endl;
      return;
    }

    std::set<std::string> delta_resources;
    std::set<std::string> removed_resources;

    if (ads_stream_ctx_ptr->inc_resource_versions[VH]
                          ->find(resource_name) ==
        ads_stream_ctx_ptr->inc_resource_versions[VH]
                          ->end())
      ads_stream_ctx_ptr->inc_resource_versions[VH]->insert(
        std::pair<std::string, std::string>(resource_name, ""));
    add_resource(resource_name, V3_VIRTUALHOST);
    delta_resources.insert(resource_name);

    return SendADSResponseIncremental(V3_VIRTUALHOST,
                                      &delta_resources, &removed_resources);
  }
  */

  std::set<std::string> listeners;
  std::set<std::string> secrets;
  std::set<std::string> virtual_hosts;

  for (auto r : resource_names) {
    if (!is_resource_present(r)) {
      std::cout << "Resource with name \"" << r << "\" not found!"
                << std::endl;
      return;
    }

    resource = &(ads_resources.at(r));
    if (V3_LISTENER == resource->type_url) {
      // Ensure this resource is available in stream context in case of
      // stream context re-initialization because of Envoy restart
      // or a Envoy made a new connection after closing the old one.
      if (ads_stream_ctx_ptr->inc_resource_versions[LISTENER]->find(r) ==
          ads_stream_ctx_ptr->inc_resource_versions[LISTENER]->end())
        ads_stream_ctx_ptr->inc_resource_versions[LISTENER]->insert(
          std::pair<std::string, std::string>(r, ""));
      listeners.insert(r);
    } else if (V3_SECRET == resource->type_url) {
      // Ensure this resource is available in stream context in case of
      // stream context re-initialization because of Envoy restart
      // or a Envoy made a new connection after closing the old one.
      if (ads_stream_ctx_ptr->inc_resource_versions[SECRET]->find(r) ==
          ads_stream_ctx_ptr->inc_resource_versions[SECRET]->end())
        ads_stream_ctx_ptr->inc_resource_versions[SECRET]->insert(
          std::pair<std::string, std::string>(r, ""));
      secrets.insert(r);
    } else if (V3_VIRTUALHOST == resource->type_url) {
      // Ensure this resource is available in stream context in case of
      // stream context re-initialization because of Envoy restart
      // or a Envoy made a new connection after closing the old one.
      if (ads_stream_ctx_ptr->inc_resource_versions[VH]->find(r) ==
          ads_stream_ctx_ptr->inc_resource_versions[VH]->end())
        ads_stream_ctx_ptr->inc_resource_versions[VH]->insert(
          std::pair<std::string, std::string>(r, ""));
      virtual_hosts.insert(r);
    }
  }

  std::cout << "Updating " << listeners.size() << " listeners, "
            << secrets.size() << " secrets, and "
            << virtual_hosts.size() << " virtual_hosts." << std::endl;

  if (listeners.size()) {
    if (!is_delta_stream) {
      SendADSResponseSotW(V3_LISTENER);
    } else {
      std::set<std::string> delta_resources;
      std::set<std::string> removed_resources;

      if ("update" == operation) {
        delta_resources = listeners;
      } else {
        removed_resources = listeners;
      }

#ifdef HAS_SECRETS
      secrets_acked = false;
      listener_acked = false;
#endif
      SendADSResponseIncremental(V3_LISTENER,
                                 &delta_resources, &removed_resources);
#ifdef HAS_SECRETS
      auto start = std::chrono::high_resolution_clock::now();
      if (has_secrets) {// Wait until secrets acked
        std::cout << "waiting for last secret ack.." << std::endl;
        std::unique_lock<std::mutex> lk(m1);
        cv.wait(lk, []{return secrets_acked;});
      } else {
        std::cout << "waiting for last listener ack.." << std::endl;
        std::unique_lock<std::mutex> lk(m1);
        cv.wait(lk, []{return listener_acked;});
      }
      std::chrono::duration<double> diff =
        std::chrono::high_resolution_clock::now()-start;
      std::cout << "Listener update - " << diff.count()
                << " sec." << std::endl;
#endif
    }
  }

  if (secrets.size()) {
    if (!is_delta_stream) {
      SendADSResponseSotW(V3_SECRET);
    } else {
      std::set<std::string> delta_resources;
      std::set<std::string> removed_resources;

      if ("update" == operation) {
        delta_resources = secrets;
      } else {
        removed_resources = secrets;
      }

      SendADSResponseIncremental(V3_SECRET,
                                 &delta_resources, &removed_resources);
    }
  }

  if (virtual_hosts.size()) {
    std::set<std::string> delta_resources;
    std::set<std::string> removed_resources;

    if ("update" == operation) {
      delta_resources = virtual_hosts;
    } else {
      removed_resources = virtual_hosts;
    }

    SendADSResponseIncremental(V3_VIRTUALHOST,
                               &delta_resources, &removed_resources);
  }
  return;
}

void HandleRuntimeConfig(std::string& config_file) {
  using namespace std::chrono_literals;

  YAML::Node config = YAML::LoadFile(config_file);
  std::vector<std::string> listener{"listener1"};
  std::vector<std::string> listeners;

  std::cout << "Config: " << std::endl;
  std::cout << "- Initial load: " << config["initial_load"].as<std::string>()
            << std::endl;
  std::cout << "- Secrets updates: " << config["secrets_updates"].as<int>()
            << std::endl;
  std::cout << "- Listener updates: ";
  for (YAML::const_iterator it = config["listener_updates"].begin();
       it != config["listener_updates"].end(); ++it) {
    std::cout << it->as<std::string>() << ";";
    listeners.push_back(it->as<std::string>());
  }
  std::cout << std::endl;

  secrets_acked = false;
  fs::copy("xds/lds/ilds/listener.yaml."
             + config["initial_load"].as<std::string>(),
           "xds/lds/listener1.yaml",
           fs::copy_options::overwrite_existing);
  std::cout << "Started adding a listener as initial_load with "
            << config["initial_load"].as<std::string>() << " secrets.."
            << std::endl;
#ifdef HAS_SECRETS
  HandleDynamicUpdate("add_listener", listener, true);
#else
  HandleDynamicUpdate("add_listener", listener);
#endif
  auto start = std::chrono::high_resolution_clock::now();
  // Wait until secrets acked
  {
    std::unique_lock<std::mutex> lk(m1);
    cv.wait(lk, []{return secrets_acked;});
  }
  std::chrono::duration<double> diff =
    std::chrono::high_resolution_clock::now()-start;
  std::cout << "Listener moved to active state after " << diff.count()
            << " sec." << std::endl;

  total_secrets = config["initial_load"].as<int>();

  std::promise<void> done;
  std::shared_future<void> done_future(done.get_future());

  UpdateInfo l_update_info = {LISTENER, listener, listeners, listeners.size(),
                              0, 0};
  Updater lu(done_future, 600s, l_update_info);

  UpdateInfo s_update_info = {SECRET, listener, listeners, listeners.size(), 0,
                              config["secrets_updates"].as<int>()};
  Updater su(done_future, 1s, s_update_info);

  auto l = lu.run();
  auto s = su.run();

  /*
  auto start = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff =
    std::chrono::high_resolution_clock::now()-start;
  std::cout << "elapsed " << diff.count() << " sec." << std::endl;
  */

  std::this_thread::sleep_for(48 * 3600s);
  done.set_value();
}

int main(int argc, char** argv) {
  logger = spdlog::basic_logger_mt("ads_server", "/tmp/mserver.log");
  logger->set_level(spdlog::level::trace);
  logger->set_pattern("[%m-%d %T.%e][%t][%l][%n] [%@] %v");
  spdlog::set_default_logger(logger);
  spdlog::flush_every(std::chrono::seconds(5));

  // overwrite levels from env
  //spdlog::cfg::load_env_levels();

  spdlog::info("Current log level: {}", logger->level());

  std::thread server(RunServer);

  std::string input;

  while (!server_done) {
    std::cout << "Enter \"dynamic\" to make requests." << std::endl;
    std::getline(std::cin, input);
    if ("dynamic" != input)
      continue;

    std::set<std::string> operations{"automation",
                                     "add_listener",
                                     //"add_vh",
                                     "update",
                                     "remove"};
    std::cout << "operations - ";
    for (auto o : operations)
      std::cout << o << ";";
    std::cout << std::endl;

    std::cout << "ads resources - ";
    for (auto r : ads_resources)
      std::cout << r.first << ";";
    std::cout << std::endl;

    std::string input;
#ifdef HAS_SECRETS
    std::cout << "Command syntax: OPERATION RESOURCE_NAME has_secrets/no_secrets" << std::endl
              << "  Where OPERATION can be one from the above listing." << std::endl
              << "    RESOURCE_NAME can be only from the above listing if the operation is \"update/remove\". It can be concatenated with \";\" to mention multiples resource names." << std::endl
              << "    RESOURCE_NAME must be a valid filename without extension of a yaml config file if the operation is \"add_resource\"." << std::endl
              << "    RESOURCE_NAME must be a valid yaml config filename if the operation is \"automation\"." << std::endl;
#else
    std::cout << "Command syntax: OPERATION RESOURCE_NAME" << std::endl
              << "  Where OPERATION can be one from the above listing." << std::endl
              << "    RESOURCE_NAME can be only from the above listing if the operation is \"update/remove\". It can be concatenated with \";\" to mention multiples resource names." << std::endl
              << "    RESOURCE_NAME must be a valid filename without extension of a yaml config file if the operation is \"add_resource\"." << std::endl
              << "    RESOURCE_NAME must be a valid yaml config filename if the operation is \"automation\"." << std::endl;
#endif
    std::cout << "Enter the command: ";

    std::getline(std::cin, input);

    std::vector<std::string> fields;
    split(fields, input, ' ', split::no_empties);
#ifdef HAS_SECRETS
    if (3 != fields.size()) {
#else
    if (2 != fields.size()) {
#endif
      std::cout << "Invalid command! Start over." << std::endl;
      continue;
    }
    if (operations.find(fields[0]) == operations.end()) {
      std::cout << "Invalid operation! Valid operations - ";
      for (auto o : operations)
        std::cout << o << ";";
      std::cout << " Start over." << std::endl;
      continue;
    }

    std::cout << fields[0] << " " << fields[1] << ".." << std::endl;

    if ("automation" == fields[0]) {
      HandleRuntimeConfig(fields[1]);
    } else {
      std::vector<std::string> resource_names;
      split(resource_names, fields[1], ';', split::no_empties);
#ifdef HAS_SECRETS
      HandleDynamicUpdate(fields[0], resource_names, fields[2]=="has_secrets"?true:false);
#else
      HandleDynamicUpdate(fields[0], resource_names);
#endif
    }
  }

  server.join();

  return 0;
}
