#include "ads_impl.hpp"

using envoy::service::discovery::v3::DiscoveryRequest;
using envoy::service::discovery::v3::DiscoveryResponse;
using envoy::service::discovery::v3::DeltaDiscoveryRequest;
using envoy::service::discovery::v3::DeltaDiscoveryResponse;
std::string listener_config_file, route_config_file;
std::string secret_ca_file, secret_cert_file, secret_key_file, vhds_config_file;

/* Send response over the stream for all(SotW) the subscribed resources. */
void SendADSResponseSotW(std::string type_url) {
  if (!ads_stream_ctx_ptr || !ads_stream_ptr)
    return;

  DiscoveryResponse response;
  ResourceType type;
  if (V3_LISTENER == type_url) type = LISTENER;
  else if (V3_ROUTECONFIG == type_url) type = RC;
  else if (V3_SECRET == type_url) type = SECRET;
  else if (V3_VIRTUALHOST == type_url) type = VH;

  switch (type) {
    case LISTENER: {
      for (auto r : ads_stream_ctx_ptr->sotw_resource_names) {
        envoy::config::listener::v3::Listener listener;
        listener_config_file = "xds/lds/listener1.yaml";
        // check ads has config for the resource
        if (!is_file_exists(listener_config_file))
          break;
        try {
          listener = parseYaml<envoy::config::listener::v3::Listener>(
            ReadFile(listener_config_file));
        } catch (const Envoy::EnvoyException& e) {
          ERROR("EnvoyException: {}", e.what());
          return;
        }
        response.add_resources()->PackFrom(listener);
      }
      break;
    }
    case RC: {
      for (auto r : ads_stream_ctx_ptr->sotw_resource_names) {
        envoy::config::route::v3::RouteConfiguration rc;
        route_config_file = "xds/rds/" + r + ".yaml";
        // check ads has config for the resource
        if (!is_file_exists(route_config_file))
          continue;
        try {
          rc = parseYaml<envoy::config::route::v3::RouteConfiguration>(
            ReadFile(route_config_file));
        } catch (const Envoy::EnvoyException& e) {
          ERROR("EnvoyException: {}", e.what());
          return;
        }
        response.add_resources()->PackFrom(rc);
      }
      break;
    }
    case SECRET: {
      //if (0 == ads_stream_ctx_ptr->sotw_resource_names.size())
      //  return; // No resources subscribed.

      // To ignore n - 1 requests if the requests carrying cumulative resource names
      // In this case, ignore n - 1 requests and respond to nth request that has all the resource names.
      //if (999 > ads_stream_ctx_ptr->sotw_resource_names.size())
      //  return;

      for (auto r : ads_stream_ctx_ptr->sotw_resource_names) {
        envoy::extensions::transport_sockets::tls::v3::Secret secret;
        secret.set_name(r);
        if ("trusted_ca" == r) {
          secret_ca_file = "xds/sds/ca.crt";
          if (!is_file_exists(secret_ca_file))
            continue;
          auto* validation_context = secret.mutable_validation_context();
          validation_context->mutable_trusted_ca()->set_inline_string(
            ReadFile(secret_ca_file));
        } else {
          secret_cert_file = "xds/sds/" + r + "/" + r + ".crt";
          secret_key_file = "xds/sds/" + r + "/" + r + ".key";
          if (!(is_file_exists(secret_cert_file) && is_file_exists(secret_key_file)))
            continue;
          auto* tls_certificate = secret.mutable_tls_certificate();
          tls_certificate->mutable_certificate_chain()->set_inline_string(
            ReadFile(secret_cert_file));
          tls_certificate->mutable_private_key()->set_inline_string(
            ReadFile(secret_key_file));
        }
        response.add_resources()->PackFrom(secret);
      }
      break;
    }
    // VHDS doesn't support SotW stream. It works only with Delta stream.
    case VH: {break;}
  }

  ads_stream_ctx_ptr->sotw_version[type] = VersionInfo();
  (ads_stream_ctx_ptr->nonce[type])++;
  response.set_version_info(ads_stream_ctx_ptr->sotw_version[type]);
  response.set_nonce(std::to_string(ads_stream_ctx_ptr->nonce[type]));
  response.set_type_url(type_url);

  // Log response
  TRACE(ReprDiscoveryResponse(response));

  // TODO: Use lock when invoking this on-demand.
  (
    (grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>*)
    ads_stream_ptr
  )->Write(response);
}

void HandleStreamRequest(DiscoveryRequest& request) {
  ResourceType type;
  // VHDS doesn't support SotW stream. It works only with Delta stream.
  if (V3_LISTENER == request.type_url()) {
    type = LISTENER;
    // modified locally
    request.mutable_resource_names()->Add("listener1");
  } else if (V3_ROUTECONFIG == request.type_url()) {
    type = RC;
  } else if (V3_SECRET == request.type_url()) {
    type = SECRET;
  }

  // Analyze request for ACK/NACK based on version info and nonce
  if ("" == request.version_info() &&
      "" == request.response_nonce()) { // Initial request
    INFO("Initial request");
  } else if (std::to_string(ads_stream_ctx_ptr->nonce[type]) ==
             request.response_nonce()) {
    if (ads_stream_ctx_ptr->sotw_version[type] == request.version_info()) {
      // ACK
      INFO("ACK");
      return;
    } else { // NACK
      INFO("NACK");
    }
  } else {
    if (0 == ads_stream_ctx_ptr->nonce[type]) {
      INFO("Initial request after ADS restart");
    } else {
      INFO("Envoy has unknown version {} of the resource, so ignoring the request",
                  request.version_info());
      return;
    }
  }

  // Update local memory with the subscribed resources
  ads_stream_ctx_ptr->sotw_resource_names = request.resource_names(); 
  for (auto r : request.resource_names())
    if (!is_resource_present(r))
      add_resource(r, request.type_url());

  // Send response
  try {
    SendADSResponseSotW(request.type_url());
  } catch (const std::runtime_error& e) {
    ERROR("sendADSResponseSotW - runtime_error: {}", e.what());
  }
}

/* State of the World (SotW) */
grpc::Status AggregatedDiscoveryServiceImpl::StreamAggregatedResources(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>* stream) {
  DEBUG("StreamAggregatedResources invoked");

  ads_stream_ptr = stream;
  is_delta_stream = false;
  // This resource_names is place holder variable
  google::protobuf::RepeatedPtrField<std::string> resource_names;
  // VHDS doesn't support SotW stream. It works only with Delta stream.
  StreamContext stream_context = { 
    { {LISTENER, "_"}, {RC, "_"}, {SECRET, "_"} },
    resource_names,
    { {LISTENER, NULL}, {RC, NULL}, {SECRET, NULL} },
    { {LISTENER, 0}, {RC, 0}, {SECRET, 0} } };
  ads_stream_ctx_ptr = &stream_context;
  DiscoveryRequest request;

  while (stream->Read(&request)) {
    // Log request
    TRACE(ReprDiscoveryRequest(request));

    HandleStreamRequest(request);
  }
  ads_stream_ptr = ads_stream_ctx_ptr = NULL;
  return grpc::Status::OK;
} 

/* Send response over the stream for delta of the subscribed resources. */
void SendADSResponseIncremental(std::string type_url,
                                std::set<std::string>* delta_resources,
                                std::set<std::string>* removed_resources) {
  if (!ads_stream_ctx_ptr || !ads_stream_ptr)
    return;

  DeltaDiscoveryResponse response;
  std::string version;
  ResourceType type;
  if (V3_LISTENER == type_url) type = LISTENER;
  else if (V3_ROUTECONFIG == type_url) type = RC;
  else if (V3_SECRET == type_url) type = SECRET;
  else if (V3_VIRTUALHOST == type_url) type = VH;

  for (auto r : *delta_resources) {
    auto resource = response.add_resources();
    bool resource_not_found = false;

    switch (type) {
      case LISTENER: {
        envoy::config::listener::v3::Listener listener;
        listener_config_file = "xds/lds/" + r + ".yaml";
        if (!is_file_exists(listener_config_file)) {
          std::cout << "resource not found: " << r << std::endl;
          resource_not_found = true;
          break;
        }
        try {
          listener = parseYaml<envoy::config::listener::v3::Listener>(
            ReadFile(listener_config_file));
        } catch (const Envoy::EnvoyException& e) {
          ERROR("EnvoyException: {}", e.what());
          return;
        }
        resource->mutable_resource()->PackFrom(listener);
        break;
      }
      case RC: {
        envoy::config::route::v3::RouteConfiguration rc;
        route_config_file = "xds/rds/" + r + ".yaml";
        if (!is_file_exists(route_config_file)) {
          std::cout << "resource not found: " << r << std::endl;
          resource_not_found = true;
          break;
        }
        try {
          rc = parseYaml<envoy::config::route::v3::RouteConfiguration>(
            ReadFile(route_config_file));
        } catch (const Envoy::EnvoyException& e) {
          ERROR("EnvoyException: {}", e.what());
          return;
        }
        resource->mutable_resource()->PackFrom(rc);
        break;
      }
      case SECRET: {
        envoy::extensions::transport_sockets::tls::v3::Secret secret;
        secret.set_name(r);
        if ("trusted_ca" == r) {
          secret_ca_file = "xds/sds/ca.crt";
          if (!is_file_exists(secret_ca_file)) {
            std::cout << "resource not found: ca.crt" << std::endl;
            resource_not_found = true;
            break;
          }
          auto* validation_context = secret.mutable_validation_context();
          validation_context->mutable_trusted_ca()->set_inline_string(
            ReadFile(secret_ca_file));
        } else {
          secret_cert_file = "xds/sds/" + r + "/" + r + ".crt";
          secret_key_file = "xds/sds/" + r + "/" + r + ".key";
          if (!(is_file_exists(secret_cert_file) && is_file_exists(secret_key_file))) {
            std::cout << "resource not found: " << r << std::endl;
            resource_not_found = true;
            break;
          }
          auto* tls_certificate = secret.mutable_tls_certificate();
          tls_certificate->mutable_certificate_chain()->set_inline_string(
            ReadFile(secret_cert_file));
          tls_certificate->mutable_private_key()->set_inline_string(
            ReadFile(secret_key_file));
        }
        resource->mutable_resource()->PackFrom(secret);
        break;
      }
      case VH: {
        envoy::config::route::v3::VirtualHost vh;
        try {
          vhds_config_file = "xds/vhds/" + r + ".yaml";
          if (!is_file_exists(vhds_config_file)) {
            std::cout << "resource not found: " << r << std::endl;
            resource_not_found = true;
            break;
          }
          vh = parseYaml<envoy::config::route::v3::VirtualHost>(
            ReadFile(vhds_config_file));
        } catch (const Envoy::EnvoyException& e) {
          ERROR("EnvoyException: {}", e.what());
          return;
        }
        resource->mutable_resource()->PackFrom(vh);
        break; 
      } 
    }

    if (resource_not_found) {
      response.mutable_resources()->RemoveLast();
      // response.mutable_resources()->Clear();
      std::cout << "Removing resource as resource data is not available" << std::endl;
      continue;
    }
    resource->set_name(r);
    version = VersionInfo();
    ads_stream_ctx_ptr->inc_resource_versions[type]->at(r) = version;
    resource->set_version(version);
  }

  *response.mutable_removed_resources() = {removed_resources->begin(),
                                           removed_resources->end()};
  (ads_stream_ctx_ptr->nonce[type])++;
  response.set_type_url(type_url);
  response.set_nonce(std::to_string(ads_stream_ctx_ptr->nonce[type]));

  // Log response
  TRACE(ReprDeltaDiscoveryResponse(response));

  // TODO: Use lock when invoking this on-demand.
  (
    (grpc::ServerReaderWriter<DeltaDiscoveryResponse, DeltaDiscoveryRequest>*)
    ads_stream_ptr
  )->Write(response);
}

void HandleDeltaRequest(DeltaDiscoveryRequest& request) {
  ResourceType type;
  std::set<std::string> delta_resources;
  std::set<std::string> removed_resources;
  if (V3_LISTENER == request.type_url()) {
    type = LISTENER;
  } else if (V3_ROUTECONFIG == request.type_url()) {
    type = RC;
  } else if (V3_SECRET == request.type_url()) {
    type = SECRET;
  } else if (V3_VIRTUALHOST == request.type_url()) {
    type = VH;
    // It is required to respond with a default virtual host
    // on first request. Without this, the http.on_demand filter will not
    // make on_demand requests for virtual hosts on http connections.
    if (!is_resource_present("vh1") ||
        0 == ads_stream_ctx_ptr->nonce[type])
      delta_resources.insert("vh1");
    if (ads_stream_ctx_ptr->inc_resource_versions[type]->find("vh1") ==
        ads_stream_ctx_ptr->inc_resource_versions[type]->end())
      ads_stream_ctx_ptr->inc_resource_versions[type]->insert(
        std::pair<std::string, std::string>("vh1", ""));
  }
 
  if ("" == request.response_nonce()) { // Initial request
    INFO("Initial request");
  } else if (0 == ads_stream_ctx_ptr->nonce[type]) {
    INFO("Initial request after ADS restart");
  } else if (std::to_string(ads_stream_ctx_ptr->nonce[type]) !=
             request.response_nonce()) {
    INFO("Request ignored as the nonce is unknown");
    return;
  } else {
    // Analyze request for ACK/NACK based on error detail
    if (0 == request.error_detail().code()) { //ACK
      INFO("ACK");
#ifdef HAS_SECRETS
      if (LISTENER == type) {
        {
          std::lock_guard<std::mutex> lk(m1);
          listener_acked = true;
        }
        cv.notify_one();
      }
#endif
      if (SECRET == type) {
        {
          std::lock_guard<std::mutex> lk(m1);
          secrets_acked = true;
        }
        cv.notify_one();
      }
      return;
    } else { // NACK
      INFO("NACK");
    }
  }

  // Process subscribed resources
  if (request.resource_names_subscribe().size())
    DEBUG("Nothing to inspect in resources subscribed");
  for (auto r : request.resource_names_subscribe()) {
    // Update the local memory with the subscribed resources
    if (ads_stream_ctx_ptr->inc_resource_versions[type]->find(r) ==
        ads_stream_ctx_ptr->inc_resource_versions[type]->end()) // New resource
      ads_stream_ctx_ptr->inc_resource_versions[type]->insert(
        std::pair<std::string, std::string>(r, ""));

    if (request.initial_resource_versions().find(r) ==
        request.initial_resource_versions().end()) { // Initial request
      INFO("Envoy don't have any version of the resource {}", r);
    } else {
      // Analyze request for ACK/NACK based on error detail
      if (0 == request.error_detail().code()) { //ACK
        if (ads_stream_ctx_ptr->inc_resource_versions[type]->at(r) ==
            request.initial_resource_versions().at(r)) {
          INFO("Envoy has the latest version {0} of the resource {1}",
                      request.initial_resource_versions().at(r), r);
        } else {
          INFO("Envoy has the previous version {0} of the resource {1}",
                      request.initial_resource_versions().at(r), r);
        }
      } else { // NACK
        INFO("Envoy has rejected the resource {} in the previous response",
                    r);
      }
    }
    // Include this resource in the response
    delta_resources.insert(r);
  }

  // Process unsubscribed resources
  if (!request.resource_names_unsubscribe().size())
    DEBUG("Nothing to inspect in resources unsubscribed");
  for (auto r : request.resource_names_unsubscribe()) {
    // Envoy is not interested with this resource in our memory
    // Update local memory by erasing this resource
    if (ads_stream_ctx_ptr->inc_resource_versions[type]->find(r) !=
        ads_stream_ctx_ptr->inc_resource_versions[type]->end()) {
      removed_resources.insert(r);
      ads_stream_ctx_ptr->inc_resource_versions[type]->erase(r);
    }

    if (is_resource_present(r))
      remove_resource(r);
  }

  for (auto r : delta_resources)
    // Update local memory with the subscribed resources
    if (!is_resource_present(r))
      add_resource(r, request.type_url());

  // Send response
  SendADSResponseIncremental(request.type_url(),
                             &delta_resources, &removed_resources);
}

/* Incremental */
grpc::Status AggregatedDiscoveryServiceImpl::DeltaAggregatedResources(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<DeltaDiscoveryResponse,
                             DeltaDiscoveryRequest>* stream) {
  DEBUG("DeltaAggregatedResources invoked");

#if 0 // inspect ServerContext
  {
    // Get the client's initial metadata
    std::cout << "Client metadata: " << std::endl;
    const std::multimap<grpc::string_ref, grpc::string_ref> metadata = context->client_metadata();
    for (auto iter = metadata.begin(); iter != metadata.end(); ++iter) {
      std::cout << "Header key: " << iter->first << ", value: ";
      // Check for binary value
      size_t isbin = iter->first.find("-bin");
      if ((isbin != std::string::npos) && (isbin + 4 == iter->first.size())) {
        std::cout <<  std::hex;
        for (auto c : iter->second) {
          std::cout << static_cast<unsigned int>(c);
        }
        std::cout <<  std::dec;
      } else {
        std::cout << iter->second;
      }
      std::cout << std::endl;
    }
  }

  {
    auto auth_context = context->auth_context();
    std::cout << "Auth Context:" << std::endl;
    std::cout << "IsPeerAuthenticated : " << auth_context->IsPeerAuthenticated() << std::endl;
    auto peer_identity = auth_context->GetPeerIdentity();
    std::cout << "Peer Identity: ";
    for (auto pi : peer_identity)
      std::cout << pi << ";";
    std::cout << std::endl;
    std::cout << "GetPeerIdentityPropertyName: " << auth_context->GetPeerIdentityPropertyName() << std::endl;
  }
#endif

  ads_stream_ptr = stream;
  is_delta_stream = true;
  // this resource_names is place holder variable
  google::protobuf::RepeatedPtrField<std::string> resource_names;
  StreamContext stream_context = { 
    { {LISTENER, ""}, {RC, ""}, {SECRET, ""}, {VH, ""} },
    resource_names,
    { {LISTENER, NULL}, {RC, NULL}, {SECRET, NULL}, {VH, NULL} },
    { {LISTENER, 0}, {RC, 0}, {SECRET, 0}, {VH, 0} } };
  ads_stream_ctx_ptr = &stream_context;
  DeltaDiscoveryRequest request;
  // Local memory that holds the active versions of each subscribed resource
  std::map<std::string, std::string> listener_resource_versions;
  std::map<std::string, std::string> rc_resource_versions;
  std::map<std::string, std::string> secret_resource_versions;
  std::map<std::string, std::string> vh_resource_versions;
  stream_context.inc_resource_versions[LISTENER] =
    &listener_resource_versions;
  stream_context.inc_resource_versions[RC] = &rc_resource_versions;
  stream_context.inc_resource_versions[SECRET] = &secret_resource_versions;
  stream_context.inc_resource_versions[VH] = &vh_resource_versions;

  while (stream->Read(&request)) {
    // Log request
    TRACE(ReprDeltaDiscoveryRequest(request));

    HandleDeltaRequest(request);
  }
  ads_stream_ptr = ads_stream_ctx_ptr = NULL;
  return grpc::Status::OK;
} 
