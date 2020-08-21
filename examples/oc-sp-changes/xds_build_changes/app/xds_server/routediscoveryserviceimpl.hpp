#include <grpc/grpc.h>

#include "envoy/service/route/v3/rds.grpc.pb.h"

class RouteDiscoveryServiceImpl final : public ::envoy::service::route::v3::RouteDiscoveryService::Service {
public:
    ::grpc::Status StreamRoutes(::grpc::ServerContext* context,
                                ::grpc::ServerReaderWriter<
                                    ::envoy::service::discovery::v3::DiscoveryResponse,
                                    ::envoy::service::discovery::v3::DiscoveryRequest
                                >* stream) override;
    
    ::grpc::Status DeltaRoutes(::grpc::ServerContext* context,
                               ::grpc::ServerReaderWriter<
                                   ::envoy::service::discovery::v3::DeltaDiscoveryResponse,
                                   ::envoy::service::discovery::v3::DeltaDiscoveryRequest
                               >* stream) override;

    ::grpc::Status FetchRoutes(::grpc::ServerContext* context,
                               const ::envoy::service::discovery::v3::DiscoveryRequest* request,
                               ::envoy::service::discovery::v3::DiscoveryResponse* response) override;
};