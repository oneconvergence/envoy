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
std::string readFile(const std::string& file_path)
{
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
std::string versionInfo()
{
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%F %T %z");
    return ss.str();
}

/* Logs the DiscoveryRequest */
void logDiscoveryRequest(DiscoveryRequest& request, bool debug)
{
    std::string log_resource_names = "";

    if (debug) {
        std::cout << request.DebugString() << std::endl;
    }
    std::cout << "    version_info " << request.version_info()
              << std::endl;
    std::cout << "    node.id " << request.node().id() << std::endl;
    std::cout << "    node.cluster " << request.node().cluster()
              << std::endl;
    std::cout << "    type_url " << request.type_url() << std::endl;
    for (auto r : request.resource_names()) {
        log_resource_names += (r + ";");
    }
    std::cout << "    resource_names " << log_resource_names << std::endl;
    std::cout << "    response_nonce " << request.response_nonce()
              << std::endl;
    std::cout << "    error_detail.code " << request.error_detail().code()
              << std::endl;
    std::cout << "    error_detail.message "
              << request.error_detail().message() << std::endl;
}

/* Logs the DiscoveryResponse */
void logDiscoveryResponse(DiscoveryResponse& response, bool debug)
{
    if (debug) {
        std::cout << response.DebugString() << std::endl;
    }
    std::cout << "    version_info " << response.version_info() << std::endl;
    std::cout << "    type_url " << response.type_url() << std::endl;
    std::cout << "    total resources " << response.mutable_resources()->size() << std::endl;
    std::cout << "    nonce " << response.nonce() << std::endl;
}

/* Logs the DeltaDiscoveryRequest */
void logDeltaDiscoveryRequest(DeltaDiscoveryRequest& request, bool debug)
{
    std::string log_resource_names = "";

    if (debug) {
        std::cout << request.DebugString() << std::endl;
    }
    std::cout << "    node().id() "
              << request.node().id() << std::endl;
    std::cout << "    node().cluster() "
              << request.node().cluster() << std::endl;
    std::cout << "    type_url() "
              << request.type_url() << std::endl;
    std::cout << "    initial_resource_versions().size() "
              << request.initial_resource_versions().size() << std::endl;
    std::cout << "    resource_names_subscribe().size() "
              << request.resource_names_subscribe().size() << std::endl;
    for (auto r : request.resource_names_subscribe()) {
        log_resource_names += (r + ";");
    }
    std::cout << "    resource_names_subscribe "
              << log_resource_names << std::endl;
    std::cout << "    resource_names_unsubscribe().size() "
              << request.resource_names_unsubscribe().size() << std::endl;
    log_resource_names = "";
    for (auto r : request.resource_names_unsubscribe()) {
        log_resource_names += (r + ";");
    }
    std::cout << "    resource_names_unsubscribe "
              << log_resource_names << std::endl;
    std::cout << "    response_nonce() "
              << request.response_nonce() << std::endl;
    std::cout << "    error_detail().code() "
              << request.error_detail().code() << std::endl;
    std::cout << "    error_detail().message() "
              << request.error_detail().message() << std::endl;
}

/* Logs the DeltaDiscoveryResponse */
void logDeltaDiscoveryResponse(DeltaDiscoveryResponse& response, bool debug)
{
    if (debug) {
        std::cout << response.DebugString() << std::endl;
    }
    std::cout << "    type_url() " << response.type_url() << std::endl;
    std::cout << "    nonce() " << response.nonce() << std::endl;
}
