#include <grpc/grpc.h>

#include "envoy/service/listener/v3/lds.grpc.pb.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/config/listener/v3/listener.pb.validate.h"
#include "envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.pb.h"
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"

#include "resource.hpp"
#include "util.hpp"

void sendLDSResponseSotW(StreamContext* stream_context, std::string type_url);
void sendLDSResponseIncremental(
    StreamContext* stream_context,
    std::string type_url,
    std::set<std::string>* delta_resources,
    std::set<std::string>* removed_resources);

class ListenerDiscoveryServiceImpl final : public envoy::service::listener::v3::ListenerDiscoveryService::Service {
public:
    grpc::Status StreamListeners(grpc::ServerContext* context,
                                 grpc::ServerReaderWriter<
                                     envoy::service::discovery::v3::DiscoveryResponse,
                                     envoy::service::discovery::v3::DiscoveryRequest
                                 >* stream) override;
    
    grpc::Status DeltaListeners(grpc::ServerContext* context,
                                grpc::ServerReaderWriter<
                                    envoy::service::discovery::v3::DeltaDiscoveryResponse,
                                    envoy::service::discovery::v3::DeltaDiscoveryRequest
                                >* stream) override;

    grpc::Status FetchListeners(grpc::ServerContext* context,
                                const envoy::service::discovery::v3::DiscoveryRequest* request,
                                envoy::service::discovery::v3::DiscoveryResponse* response) override;
};
