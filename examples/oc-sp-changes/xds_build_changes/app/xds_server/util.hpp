#ifndef UTIL_HPP
#define UTIL_HPP

#include <sstream>
#include <string>

#include "envoy/common/exception.h"
#include "common/config/version_converter.h"
#include "common/protobuf/utility.h"
#include "common/protobuf/message_validator_impl.h"
#include "envoy/service/discovery/v3/discovery.pb.h"

std::string readFile(const std::string& file_path);
std::string versionInfo();
void logDiscoveryRequest(
    envoy::service::discovery::v3::DiscoveryRequest& request, bool debug);
void logDiscoveryResponse(
    envoy::service::discovery::v3::DiscoveryResponse& response, bool debug);
void logDeltaDiscoveryRequest(
    envoy::service::discovery::v3::DeltaDiscoveryRequest& request, bool debug);
void logDeltaDiscoveryResponse(
    envoy::service::discovery::v3::DeltaDiscoveryResponse& response,
    bool debug);

struct split
{
  enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(
  Container&                                 result,
  const typename Container::value_type&      s,
  typename Container::value_type::value_type delimiter,
  split::empties_t                           empties = split::empties_ok )
{
  result.clear();
  std::istringstream ss( s );
  while (!ss.eof())
  {
    typename Container::value_type field;
    getline( ss, field, delimiter );
    if ((empties == split::no_empties) && field.empty()) continue;
    result.push_back( field );
  }
  return result;
}

template <class MessageType>
static MessageType parseYaml(const std::string& yaml)
{
    MessageType message;
    Envoy::MessageUtil::loadFromYamlAndValidate(
        yaml, message, Envoy::ProtobufMessage::getStrictValidationVisitor());
    Envoy::Config::VersionConverter::eraseOriginalTypeInformation(message);
    return message;
}

#endif
