#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <map>
#include <string>
#include <google/protobuf/repeated_field.h>

// StreamContext to maintain the stream/RPC session level common data
typedef struct StreamContext_ {
    std::string stream_type;
    void* stream;
    std::string sotw_version;
    google::protobuf::RepeatedPtrField<std::string> sotw_resource_names;
    std::map<std::string, std::string>* inc_resource_versions;
    uint64_t nonce;
} StreamContext;

// Per resource stream identification and version(only for Delta)
typedef struct Resource_ {
    StreamContext* stream_context;
    std::string type_url;
    std::string inc_version;
} Resource;

// Global map that maitains the resources information that
// Envoy subscribed with the LDS server. This is used in
// making on-demand response to Envoy on any updates
extern std::map<std::string, Resource> xds_resources;

void add_resource(const std::string& name, const std::string& type_url, StreamContext& stream_context);
void remove_resource(const std::string& name);
bool is_resource_present(const std::string& name);

#endif
