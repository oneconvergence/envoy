#include "resource.hpp"

void add_resource(const std::string& name,
                  const std::string& type_url) {
   Resource resource = {
     type_url,
     "" // inc_version
   };
   ads_resources.insert(
     std::pair<std::string, Resource>(name, resource));
}

void remove_resource(const std::string& name) {
   ads_resources.erase(name);
}

bool is_resource_present(const std::string& name) {
  return (ads_resources.find(name) != ads_resources.end());
}
