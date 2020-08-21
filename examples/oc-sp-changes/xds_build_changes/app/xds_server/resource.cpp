#include "resource.hpp"

void add_resource(const std::string& name, const std::string& type_url, StreamContext& stream_context)
{
     Resource resource = {
         &stream_context,
         type_url,
         "" // inc_version
     };
     xds_resources.insert(
         std::pair<std::string, Resource>(name, resource));
}

void remove_resource(const std::string& name)
{
     xds_resources.erase(name);
}

bool is_resource_present(const std::string& name)
{
    return (xds_resources.find(name) != xds_resources.end());
}
