#include "listenerdiscoveryserviceimpl.hpp"

using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::service::discovery::v3::DeltaDiscoveryRequest;
using envoy::service::discovery::v3::DeltaDiscoveryResponse;

/* Static listener
envoy::config::listener::v3::Listener listener1_listener()
{
    envoy::config::listener::v3::Listener listener;
    listener.set_name("listener1");
    listener.mutable_address()->mutable_socket_address()
                              ->set_address("0.0.0.0");
    listener.mutable_address()->mutable_socket_address()
                              ->set_port_value(443);
    listener.add_listener_filters();
    listener.mutable_listener_filters(0)->set_name("envoy.filters.listener.tls_inspector");

    envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager hcm;
    hcm.set_codec_type(envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::AUTO);
    hcm.set_stat_prefix("ingress_http");
    hcm.mutable_route_config()->set_name("local_route");
    hcm.mutable_route_config()->add_virtual_hosts();
    hcm.mutable_route_config()->mutable_virtual_hosts(0)
                              ->set_name("backend");
    hcm.mutable_route_config()->mutable_virtual_hosts(0)
                              ->add_domains("service1.ocenvoy.com");
    hcm.mutable_route_config()->mutable_virtual_hosts(0)
                              ->add_routes();
    hcm.mutable_route_config()->mutable_virtual_hosts(0)
                              ->mutable_routes(0)
                              ->mutable_match()
                              ->set_prefix("/service/1");
    hcm.mutable_route_config()->mutable_virtual_hosts(0)
                              ->mutable_routes(0)
                              ->mutable_route()
                              ->set_cluster("service1");
    hcm.add_http_filters();
    hcm.mutable_http_filters(0)->set_name("envoy.filters.http.router");

    listener.add_filter_chains();
    listener.mutable_filter_chains(0)->add_filters();
    listener.mutable_filter_chains(0)->mutable_filters(0)
                                     ->set_name("envoy.filters.network.http_connection_manager");
    listener.mutable_filter_chains(0)->mutable_filters(0)
                                     ->mutable_typed_config()
                                     ->PackFrom(hcm);

    listener.mutable_filter_chains(0)->mutable_filter_chain_match()
                                     ->add_server_names("service1.ocenvoy.com");

    envoy::extensions::transport_sockets::tls::v3::DownstreamTlsContext dtc;
    dtc.mutable_common_tls_context()->add_tls_certificate_sds_secret_configs();
    dtc.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs(0)
                                    ->set_name("service1.ocenvoy.com");
    dtc.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs(0)
                                    ->mutable_sds_config()
                                    ->set_resource_api_version(envoy::config::core::v3::ApiVersion::V3);
    dtc.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs(0)
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->set_api_type(envoy::config::core::v3::ApiConfigSource::GRPC);
    dtc.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs(0)
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->set_transport_api_version(envoy::config::core::v3::ApiVersion::V3);
    dtc.mutable_common_tls_context()->mutable_tls_certificate_sds_secret_configs(0)
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->add_grpc_services()
                                    ->mutable_envoy_grpc()
                                    ->set_cluster_name("xds_cluster");
    dtc.mutable_common_tls_context()->mutable_validation_context_sds_secret_config()
                                    ->set_name("trusted_ca");
    dtc.mutable_common_tls_context()->mutable_validation_context_sds_secret_config()
                                    ->mutable_sds_config()
                                    ->set_resource_api_version(envoy::config::core::v3::ApiVersion::V3);
    dtc.mutable_common_tls_context()->mutable_validation_context_sds_secret_config()
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->set_api_type(envoy::config::core::v3::ApiConfigSource::DELTA_GRPC);
    dtc.mutable_common_tls_context()->mutable_validation_context_sds_secret_config()
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->set_transport_api_version(envoy::config::core::v3::ApiVersion::V3);
    dtc.mutable_common_tls_context()->mutable_validation_context_sds_secret_config()
                                    ->mutable_sds_config()
                                    ->mutable_api_config_source()
                                    ->add_grpc_services()
                                    ->mutable_envoy_grpc()
                                    ->set_cluster_name("xds_cluster");

    listener.mutable_filter_chains(0)->mutable_transport_socket()
                                     ->set_name("envoy.transport_sockets.tls");
    listener.mutable_filter_chains(0)->mutable_transport_socket()
                                     ->mutable_typed_config()
                                     ->PackFrom(dtc);

    return listener;
}
*/

/* Send response over the stream for all(SotW) the subscribed resources. */
void sendLDSResponseSotW(StreamContext* stream_context, std::string type_url)
{
    DiscoveryResponse response;

    envoy::config::listener::v3::Listener listener_config;
    try {
        listener_config = 
            parseYaml<envoy::config::listener::v3::Listener>(
                readFile("lds/listener1.yaml"));
    } catch (const Envoy::EnvoyException& e) {
        std::cout << "EnvoyException: " << e.what() << std::endl;
        return;
    }
    response.add_resources()->PackFrom(listener_config);

    stream_context->sotw_version = versionInfo();
    (stream_context->nonce)++;
    response.set_version_info(stream_context->sotw_version);
    response.set_type_url(type_url);
    response.set_nonce(std::to_string(stream_context->nonce));

    // Log response
    std::cout << "Response:" << std::endl;
    logDiscoveryResponse(response, false);

    // TODO: Use lock when invoking this on-demand.
    (
        (grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>*)
        (stream_context->stream)
    )->Write(response);
}

/* State of the World (SotW) */
grpc::Status ListenerDiscoveryServiceImpl::StreamListeners(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>* stream)
{
    std::cout << std::endl << "StreamListeners invoked." << std::endl;

    // This resource_names is place holder variable
    google::protobuf::RepeatedPtrField<std::string> resource_names;
    StreamContext stream_context = { "SotW", stream, "_", resource_names,
                                     NULL, 0 };
    DiscoveryRequest request;

    while (stream->Read(&request)) {
        // Log request
        std::cout << "Request:" << std::endl;
        logDiscoveryRequest(request, false);

        // Analyze request for ACK/NACK based on version info and nonce
        if (0 == request.version_info().compare("") &&
            0 == request.response_nonce().compare("")) { // Initial request
            std::cout << "Initial request." << std::endl;
        } else if (0 == request.response_nonce().compare(
                       std::to_string(stream_context.nonce))) {
            if (0 == request.version_info().compare(
                    stream_context.sotw_version)) { // ACK
                std::cout << "ACK." << std::endl;
                continue;
            } else { // NACK
                std::cout << "NACK." << std::endl;
            }
        } else {
            if (0 == stream_context.nonce) {
                std::cout << "Initial request after xDS restart." << std::endl;
            } else {
                std::cout << "Envoy has unknown version "
                          << request.version_info() << " of the resource."
                          << " Ignoring the request."
                          << std::endl;
                continue;
            }
        }

        if (!is_resource_present("listener1")) {
            add_resource("listener1", request.type_url(), stream_context);
        }
 
        // Send response
        sendLDSResponseSotW(&stream_context, request.type_url());
    }

    return grpc::Status::OK;
} 

/* Send response over the stream for delta of the subscribed resources. */
void sendLDSResponseIncremental(
    StreamContext* stream_context,
    std::string type_url,
    std::set<std::string>* delta_resources,
    std::set<std::string>* removed_resources)
{
    DeltaDiscoveryResponse response;
    std::string version;

    if (0 == delta_resources->size()
        && 0 == removed_resources->size()) {
        // No delta/removed resources.
        return;
    }

    for (auto r : *delta_resources) {
        envoy::config::listener::v3::Listener listener_config;
        try {
            listener_config = 
                parseYaml<envoy::config::listener::v3::Listener>(
                    readFile("lds/" + r + ".yaml"));
        } catch (const Envoy::EnvoyException& e) {
            std::cout << "EnvoyException: " << e.what() << std::endl;
            return;
        }
        auto resource = response.add_resources();
        resource->set_name(r);
        version = versionInfo();
        stream_context->inc_resource_versions->at(r) = version;
        resource->set_version(version);
        resource->mutable_resource()->PackFrom(listener_config);
    }

    *response.mutable_removed_resources() = {removed_resources->begin(),
                                             removed_resources->end()};
    (stream_context->nonce)++;
    response.set_type_url(type_url);
    response.set_nonce(std::to_string(stream_context->nonce));

    // Log response
    std::cout << "Response:" << std::endl;
    logDeltaDiscoveryResponse(response, false);

    // TODO: Use lock when invoking this on-demand.
    (
        (grpc::ServerReaderWriter<DeltaDiscoveryResponse,
                                  DeltaDiscoveryRequest>*)
        (stream_context->stream)
    )->Write(response);
}

/* Incremental */
grpc::Status ListenerDiscoveryServiceImpl::DeltaListeners(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DeltaDiscoveryResponse,
                             DeltaDiscoveryRequest>* stream)
{
    std::cout << std::endl << "DeltaListeners invoked." << std::endl;

    // this resource_names is place holder variable
    google::protobuf::RepeatedPtrField<std::string> resource_names;
    StreamContext stream_context = { "Incremental", stream, "", resource_names,
                                      NULL, 0 };
    DeltaDiscoveryRequest request;
    // Local memory that holds the active versions of each subscribed resource
    std::map<std::string, std::string> resource_versions;
    stream_context.inc_resource_versions = &resource_versions;
    // The resources out of Envoy subscribed resources,
    // that needs to be included in response
    std::set<std::string> delta_resources;
    std::set<std::string> removed_resources;

    while (stream->Read(&request)) {
        // Log request
        std::cout << "Request:" << std::endl;
        logDeltaDiscoveryRequest(request, false);

        if (0 == request.response_nonce().compare("")) { // Initial request
            std::cout << "Initial request." << std::endl;
        } else if (0 == stream_context.nonce) {
            std::cout << "Initial request after xDS restart." << std::endl;
        } else if (0 != request.response_nonce()
                               .compare(std::to_string(
                                            stream_context.nonce))) {
            std::cout << "Ignore request as the nonce is unknown." << std::endl;
            continue;
        } else {
            // Analyze request for ACK/NACK based on error detail
            if (0 == request.error_detail().code()) { //ACK
                std::cout << "ACK." << std::endl;
                continue;
            } else { // NACK
                std::cout << "NACK." << std::endl;
            }
        }

        delta_resources.clear();
        // Process subscribed resources
        if (request.resource_names_subscribe().size()) {
            std::cout << "Inspecting resources subscribed." << std::endl;
        } else {
            std::cout << "Nothing to inspect in resources subscribed."
                      << std::endl;
        }
        for (auto r : request.resource_names_subscribe()) {
            // Update the local memory with the subscribed resources
            if (resource_versions.find(r) == resource_versions.end()) {
                // New resource
                resource_versions.insert(
                    std::pair<std::string, std::string>(r, ""));
            }

            if (request.initial_resource_versions().find(r)
                == request.initial_resource_versions().end()) { // Initial request
                std::cout << "Resource name: " << r
                          << ". Envoy don't have any version of this resource."
                          << std::endl;
            } else {
                // Analyze request for ACK/NACK based on error detail
                if (0 == request.error_detail().code()) { //ACK
                    if (0 == request.initial_resource_versions().at(r).compare(
                            resource_versions.at(r))) {
                        std::cout << "Resource name: " << r
                                  << ". Envoy has the latest version "
                                  << request.initial_resource_versions().at(r)
                                  << " of this resource." << std::endl;
                    } else {
                        std::cout << "Resource name: " << r
                                  << ". Envoy has the previous version "
                                  << request.initial_resource_versions().at(r)
                                  << " of this resource." << std::endl;
                    }
                } else { // NACK
                    std::cout << "Resource name: " << r
                              << ". Envoy has rejected this resource"
                              << " in the previous response." << std::endl;
                }
            }
            // Include this resource in the response
            delta_resources.insert(r);

            // Update local memory with the subscribed resources
            if (!is_resource_present(r)) {
                add_resource(r, request.type_url(), stream_context);
            }
        }

        // Process unsubscribed resources
        if (request.resource_names_unsubscribe().size()) {
            std::cout << "Inspecting resources unsubscribed." << std::endl;
        } else {
            std::cout << "Nothing to inspect in resources unsubscribed."
                      << std::endl;
        }
        for (auto r : request.resource_names_unsubscribe()) {
            // Envoy is not interested with this resource in our memory
            // Update local memory by erasing this resource
            if (resource_versions.find(r) != resource_versions.end()) {
                resource_versions.erase(r);
            }
            if (is_resource_present(r)) {
                remove_resource(r);
            }
        }

        // Update local memory with the required resource names
        // This will add these resources if Envoy requests with
        // wildcard subscribed resource names
        if (std::find(delta_resources.begin(), delta_resources.end(),
                      "listener1")
            == delta_resources.end()) {
            if (resource_versions.find("listener1")
                == resource_versions.end()) {
                // New resource
                resource_versions.insert(
                    std::pair<std::string, std::string>("listener1", ""));
            }
            if (!is_resource_present("listener1")) {
                add_resource("listener1", request.type_url(),
                             stream_context);
            }                           
            delta_resources.insert("listener1");
        }

        // Send response
        sendLDSResponseIncremental(&stream_context, request.type_url(),
                                   &delta_resources, &removed_resources);
    }

    return grpc::Status::OK;
} 

grpc::Status ListenerDiscoveryServiceImpl::FetchListeners(
    grpc::ServerContext* context,
    const DiscoveryRequest* request,
    DiscoveryResponse* response)
{
    std::cout << "FetchListeners invoked." << std::endl;
    std::cout << request->DebugString() << std::endl;
    return grpc::Status::OK;
}
