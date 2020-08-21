#ifndef UTIL_HPP
#define UTIL_HPP

#include <condition_variable>
#include <exception>
#include <mutex>
#include <sstream>
#include <string>

#include "common/protobuf/utility.h"
#include "common/protobuf/message_validator_impl.h"
#include "common/config/version_converter.h"
#include "envoy/common/exception.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
//#include "spdlog/cfg/env.h"

#define TRACE(...)     logger->trace(__VA_ARGS__)
#define DEBUG(...)     logger->debug(__VA_ARGS__)
#define INFO(...)      logger->info(__VA_ARGS__)
#define WARN(...)      logger->warn(__VA_ARGS__)
#define ERROR(...)     logger->error(__VA_ARGS__)
#define CRITICAL(...)  logger->critical(__VA_ARGS__)

extern std::shared_ptr<spdlog::logger> logger;
extern std::mutex m1;
extern std::condition_variable cv;
extern bool secrets_acked;
#ifdef HAS_SECRETS
extern bool listener_acked;
#endif

std::string ReadFile(const std::string& file_path);
std::string VersionInfo();
std::string ReprDiscoveryRequest(
  envoy::service::discovery::v3::DiscoveryRequest& request);
std::string ReprDiscoveryResponse(
  envoy::service::discovery::v3::DiscoveryResponse& response);
std::string ReprDeltaDiscoveryRequest(
  envoy::service::discovery::v3::DeltaDiscoveryRequest& request);
std::string ReprDeltaDiscoveryResponse(
  envoy::service::discovery::v3::DeltaDiscoveryResponse& response);
bool is_file_exists(const std::string& fileName);

struct split {
  enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(
    Container& result,
    const typename Container::value_type& s,
    typename Container::value_type::value_type delimiter,
    split::empties_t empties = split::empties_ok) {
  result.clear();
  std::istringstream ss(s);
  while (!ss.eof()) {
    typename Container::value_type field;
    getline(ss, field, delimiter);
    if ((empties == split::no_empties) && field.empty()) continue;
    result.push_back(field);
  }
  return result;
}

template <class MessageType>
static MessageType parseYaml(const std::string& yaml) {
  MessageType message;
  Envoy::MessageUtil::loadFromYamlAndValidate(
    yaml, message, Envoy::ProtobufMessage::getStrictValidationVisitor());
  Envoy::Config::VersionConverter::eraseOriginalTypeInformation(message);
  return message;
}

#endif
