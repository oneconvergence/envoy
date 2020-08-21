#include <grpc/grpc.h>

#include "envoy/service/secret/v3/sds.grpc.pb.h"
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"

#include "resource.hpp"
#include "util.hpp"

void sendSDSResponseSotW(StreamContext* stream_context, std::string type_url);
void sendSDSResponseIncremental(
    StreamContext* stream_context,
    std::string type_url,
    std::set<std::string>* delta_resources,
    std::set<std::string>* removed_resources);

class SecretDiscoveryServiceImpl final : public envoy::service::secret::v3::SecretDiscoveryService::Service {
public:
    grpc::Status StreamSecrets(grpc::ServerContext* context,
                               grpc::ServerReaderWriter<
                                  envoy::service::discovery::v3::DiscoveryResponse,
                                  envoy::service::discovery::v3::DiscoveryRequest
                               >* stream) override;
    
    grpc::Status DeltaSecrets(grpc::ServerContext* context,
                              grpc::ServerReaderWriter<
                                  envoy::service::discovery::v3::DeltaDiscoveryResponse,
                                  envoy::service::discovery::v3::DeltaDiscoveryRequest
                              >* stream) override;

    grpc::Status FetchSecrets(grpc::ServerContext* context,
                              const envoy::service::discovery::v3::DiscoveryRequest* request,
                              envoy::service::discovery::v3::DiscoveryResponse* response) override;
};
