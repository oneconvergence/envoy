#include "secretdiscoveryserviceimpl.hpp"

using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::service::discovery::v3::DeltaDiscoveryRequest;
using envoy::service::discovery::v3::DeltaDiscoveryResponse;


/* Send response over the stream for all(SotW) the subscribed resources. */
void sendSDSResponseSotW(StreamContext* stream_context, std::string type_url)
{
    DiscoveryResponse response;

    if (0 == stream_context->sotw_resource_names.size()) {
        // No resources subscribed.
        return;
    }

    for (auto r : stream_context->sotw_resource_names) {
        envoy::extensions::transport_sockets::tls::v3::Secret secret;
        secret.set_name(r);
        if (0 == r.compare("trusted_ca")) {
            auto* validation_context = secret.mutable_validation_context();
            validation_context->mutable_trusted_ca()->set_inline_string(
                readFile("secret/ca.crt"));
        } else {
            auto* tls_certificate = secret.mutable_tls_certificate();
            tls_certificate->mutable_certificate_chain()->set_inline_string(
                readFile("secret/" + r + "/" + r + ".crt"));
            tls_certificate->mutable_private_key()->set_inline_string(
                readFile("secret/" + r + "/" + r + ".key"));
        }
        response.add_resources()->PackFrom(secret);
    }

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
grpc::Status SecretDiscoveryServiceImpl::StreamSecrets(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>* stream)
{
    std::cout << std::endl << "StreamSecrets invoked." << std::endl;

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

        // Update local memory with the subscribed resources
        stream_context.sotw_resource_names = request.resource_names(); 
        for (auto r : request.resource_names()) {
            if (!is_resource_present(r)) {
                add_resource(r, request.type_url(), stream_context);
            }
        }

        /* FIXME
        std::vector<std::string> resources_not_subscribed;
        for (auto r : xds_resources) {
            if (0 != r.second.type_url.substr(r.second.type_url.size()-6)
                                      .compare("Secret")) {
                continue;
            }
            if (std::find(request.resource_names().begin(),
                          request.resource_names().end(),
                          r.first) == request.resource_names().end()) {
                // Envoy is not interested with this resource in our memory
                resources_not_subscribed.push_back(r.first);
            }
        }
        for (auto r : resources_not_subscribed) {
            xds_resources.erase(r);
        }
        */

        // Send response
        try {
            sendSDSResponseSotW(&stream_context, request.type_url());
        } catch (const std::runtime_error& e) {
            std::cout << "sendSDSResponseSotW - runtime_error: " << e.what() << std::endl;
        }
    }

    return grpc::Status::OK;
} 

/* Send response over the stream for delta of the subscribed resources. */
void sendSDSResponseIncremental(
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
        envoy::extensions::transport_sockets::tls::v3::Secret secret;
        secret.set_name(r);
        if (0 == r.compare("trusted_ca")) {
            auto* validation_context = secret.mutable_validation_context();
            validation_context->mutable_trusted_ca()->set_inline_string(
                readFile("secret/ca.crt"));
        } else {
            auto* tls_certificate = secret.mutable_tls_certificate();
            tls_certificate->mutable_certificate_chain()->set_inline_string(
                readFile("secret/" + r + "/" + r +  ".crt"));
            tls_certificate->mutable_private_key()->set_inline_string(
                readFile("secret/" + r + "/" + r + ".key"));
        }
        auto resource = response.add_resources();
        resource->set_name(secret.name());
        version = versionInfo();
        stream_context->inc_resource_versions->at(r) = version;
        resource->set_version(version);
        resource->mutable_resource()->PackFrom(secret);
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
grpc::Status SecretDiscoveryServiceImpl::DeltaSecrets(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DeltaDiscoveryResponse,
                             DeltaDiscoveryRequest>* stream) 
{
    std::cout << std::endl << "DeltaSecrets invoked." << std::endl;

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

        // Send response
        sendSDSResponseIncremental(&stream_context, request.type_url(),
                                   &delta_resources, &removed_resources);
    }

    return grpc::Status::OK;
} 

grpc::Status SecretDiscoveryServiceImpl::FetchSecrets(
    grpc::ServerContext* context,
    const DiscoveryRequest* request,
    DiscoveryResponse* response)
{
    std::cout << "FetchSecrets invoked." << std::endl;
    std::cout << request->DebugString() << std::endl;
    return grpc::Status::OK;
}
