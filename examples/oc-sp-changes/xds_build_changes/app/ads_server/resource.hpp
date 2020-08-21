#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <map>
#include <string>
#include <google/protobuf/repeated_field.h>

// type_url definitions
#define V3_LISTENER "type.googleapis.com/envoy.config.listener.v3.Listener"
#define V3_ROUTECONFIG "type.googleapis.com/envoy.config.route.v3.RouteConfiguration"
#define V3_SECRET "type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.Secret"
#define V3_VIRTUALHOST "type.googleapis.com/envoy.config.route.v3.VirtualHost"

// resource types
enum ResourceType { LISTENER, RC, SECRET, VH };

// StreamContext to maintain the stream/RPC session level common data
typedef struct StreamContext_ {
  std::map<ResourceType, std::string> sotw_version;
  google::protobuf::RepeatedPtrField<std::string> sotw_resource_names;
  std::map<ResourceType, std::map<std::string, std::string>*> inc_resource_versions;
  std::map<ResourceType, uint64_t> nonce;
} StreamContext;

// Per resource stream identification and version(only for Delta)
typedef struct Resource_ {
  std::string type_url;
  std::string inc_version;
} Resource;

extern void* ads_stream_ptr;
extern bool is_delta_stream;
extern StreamContext* ads_stream_ctx_ptr;
// Global map that maitains the resources information that
// Envoy subscribed with the LDS server. This is used in
// making on-demand response to Envoy on any updates
extern std::map<std::string, Resource> ads_resources;

void add_resource(const std::string& name, const std::string& type_url);
void remove_resource(const std::string& name);
bool is_resource_present(const std::string& name);

#endif
