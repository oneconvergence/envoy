#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "util.hpp"

using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::service::discovery::v3::DeltaDiscoveryRequest;
using envoy::service::discovery::v3::DeltaDiscoveryResponse;


/* Read the given file mentioned with the file path and 
 * return it's contents in string format. */
std::string ReadFile(const std::string& file_path) {
  std::ifstream src;
  src.open(file_path, std::ifstream::in | std::ifstream::binary);

  std::string contents;
  src.seekg(0, std::ios::end);
  contents.reserve(src.tellg());
  src.seekg(0, std::ios::beg);
  contents.assign((std::istreambuf_iterator<char>(src)),
                  (std::istreambuf_iterator<char>()));
  src.close();
  return contents;
}

/* Returns the timestamp in ISO date and time formats. */
std::string VersionInfo() {
  std::time_t t = std::time(nullptr);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&t), "%F %T %z");
  return ss.str();
}

/* Returns the std::string representation of DiscoveryRequest */
std::string ReprDiscoveryRequest(DiscoveryRequest& request) {
  std::string resource_names = "";
  for (auto r : request.resource_names())
    resource_names += (r + ";");

  return fmt::format("DiscoveryRequest: {{ node.id: {0}, version_info: {1}, resource_names: {2}, response_nonce: {3}, error_code: {4}, error_message: {5}, {6} }}",
                     request.node().id(), request.version_info(),
                     resource_names, request.response_nonce(),
                     request.error_detail().code(),
                     request.error_detail().message(),
                     request.type_url());
}

/* Returns the std::string representation of DiscoveryResponse */
std::string ReprDiscoveryResponse(DiscoveryResponse& response) {
  return fmt::format("DiscoveryResponse: {{ version_info: {0}, resources_size: {1}, nonce: {2}, {3} }}",
                     response.version_info(), response.resources_size(), response.nonce(),
                     response.type_url());
}

/* Returns the std::string representation of DeltaDiscoveryRequest */
std::string ReprDeltaDiscoveryRequest(DeltaDiscoveryRequest& request) {
  std::string initial_resource_versions = "";
  for (auto & kv : request.initial_resource_versions())
    initial_resource_versions += (kv.first + "=" + kv.second + ";");
  
  std::string resource_names_subscribe = "";
  for (auto r : request.resource_names_subscribe())
    resource_names_subscribe += (r + ";");

  std::string resource_names_unsubscribe = "";
  for (auto r : request.resource_names_unsubscribe())
    resource_names_unsubscribe += (r + ";");

  return fmt::format("DeltaDiscoveryRequest: {{ node.id: {0}, initial_resource_versions: {1}, resource_names_subscribe: {2}, resource_names_unsubscribe: {3}, response_nonce: {4}, error_code: {5}, error_message: {6}, {7} }}",
                     request.node().id(), initial_resource_versions,
                     resource_names_subscribe, resource_names_unsubscribe,
                     request.response_nonce(),
                     request.error_detail().code(),
                     request.error_detail().message(),
                     request.type_url());
}

/* Returns the std::string representation of DeltaDiscoveryResponse */
std::string ReprDeltaDiscoveryResponse(DeltaDiscoveryResponse& response)
{
  std::string resources = "";
  for (auto r : response.resources())
    resources += ("(" + r.name() + ", " + r.version() + ");");
 
  std::string removed_resources = "";
  for (auto r : response.removed_resources())
    removed_resources += (r + ";");

  return fmt::format("DeltaDiscoveryResponse: {{ system_version_info: {0}, resources: {1}, removed_resources: {2}, nonce: {3}, {4} }}",
                     response.system_version_info(),
                     resources, removed_resources,
                     response.nonce(), response.type_url());
}

bool is_file_exists(const std::string& fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}