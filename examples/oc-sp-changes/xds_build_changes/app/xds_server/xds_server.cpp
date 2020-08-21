#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include <thread>

#include "routediscoveryserviceimpl.hpp"
#include "virtualhostdiscoveryserviceimpl.hpp"
#include "secretdiscoveryserviceimpl.hpp"
#include "listenerdiscoveryserviceimpl.hpp"
#include "util.hpp"


std::map<std::string, Resource> xds_resources;

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    RouteDiscoveryServiceImpl rdsService;
    VirtualHostDiscoveryServiceImpl vhdService;
    SecretDiscoveryServiceImpl sdsService;
    ListenerDiscoveryServiceImpl ldsService;

    grpc::ServerBuilder builder;
    grpc::SslServerCredentialsOptions sslOpts{};
    sslOpts.client_certificate_request =
        GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
    sslOpts.pem_key_cert_pairs.push_back(
        grpc::SslServerCredentialsOptions::PemKeyCertPair{readFile("server.key"),
                                                          readFile("server.crt")});
    sslOpts.pem_root_certs = readFile("client.crt");
    auto creds = grpc::SslServerCredentials(sslOpts);
    builder.AddListeningPort(server_address, creds);

    builder.RegisterService(&rdsService);
    builder.RegisterService(&vhdService);
    builder.RegisterService(&sdsService);
    builder.RegisterService(&ldsService);
    builder.AddChannelArgument(GRPC_ARG_MAX_CONCURRENT_STREAMS, 1000000);
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

/* Utility function to invoke stream type based send response calls
 * on a resource update. */
void handle_dynamic_update(const std::string& operation,
                           const std::string& resource_name)
{
    Resource* resource;

    if (operation == "add") {
        std::cout << "Not supported!" << std::endl;
        return;
    }

    if (xds_resources.find(resource_name) == xds_resources.end()) {
        std::cout << "Resource with name \"" << resource_name << "\" not found!" << std::endl;
        return;
    }

    resource = &(xds_resources.at(resource_name));
    if (0 == resource->type_url.substr(resource->type_url.size()-8)
                               .compare("Listener")) {
        if (resource->stream_context->stream_type == "SotW") {
            if (operation == "remove") {
                std::cout << "Removal of resource " << resource_name
                          << " over the SotW stream is not supported."
                          << std::endl;
                return;
            }
            return sendLDSResponseSotW(resource->stream_context, resource->type_url);
        } else {
            std::set<std::string> delta_resources;
            std::set<std::string> removed_resources;
            if (operation == "update") {
                delta_resources.insert(resource_name);
            } else {
                removed_resources.insert(resource_name);
            }
            return sendLDSResponseIncremental(resource->stream_context,
                                              resource->type_url,
                                              &delta_resources, &removed_resources);
        }
    } else if (0 == resource->type_url.substr(resource->type_url.size()-6)
                                      .compare("Secret")) {
        if (resource->stream_context->stream_type == "SotW") {
            if (operation == "remove") {
                std::cout << "Removal of resource " << resource_name
                          << " over the SotW stream is not supported."
                          << std::endl;
                return;
            }
            return sendSDSResponseSotW(resource->stream_context, resource->type_url);
        } else {
            std::set<std::string> delta_resources;
            std::set<std::string> removed_resources;
            if (operation == "update") {
                delta_resources.insert(resource_name);
            } else {
                removed_resources.insert(resource_name);
            }
            return sendSDSResponseIncremental(resource->stream_context,
                                              resource->type_url,
                                              &delta_resources, &removed_resources);
        }
    }

    return;
}

int main(int argc, char** argv) {
    std::thread server (RunServer);

    std::string input;

    while (true) {
        std::cout << "Enter \"dynamic\" to make requests." << std::endl;
        std::getline(std::cin, input);
        if (0 != input.compare("dynamic")) {
            continue;
        }

        std::cout << "xds resources - ";
        for (auto r : xds_resources) {
            std::cout << r.first << ";";
        }
        std::cout << std::endl;

        std::string input;
        std::cout << "Command syntax: OPERATION RESOURCE_NAME" << std::endl
                  << "  Where OPERATION can be - add/update/remove" << std::endl
                  << "        RESOURCE_NAME can be any any one from the above listing." << std::endl;
        std::cout << "Enter the command: ";

        std::getline(std::cin, input);

        std::vector<std::string> fields;
        split(fields, input, ' ', split::no_empties);
        if (2 != fields.size()) {
            std::cout << "Invalid command! Start over." << std::endl;
            continue;
        }
        if (0 != fields[0].compare("add")
            && 0 != fields[0].compare("update")
            && 0 != fields[0].compare("remove")) {
            std::cout << "Invalid operation! Valid operations - (add/update/remove)."
                      << " Start over." << std::endl;
            continue;
        }

        std::cout << fields[0] << " " << fields[1] << ".." << std::endl;

        handle_dynamic_update(fields[0], fields[1]); 
    }

    server.join();

    return 0;
}
