#include <grpc/grpc.h>

#include "envoy/service/route/v3/rds.grpc.pb.h"

class VirtualHostDiscoveryServiceImpl final : public ::envoy::service::route::v3::VirtualHostDiscoveryService::Service {
public: 
    ::grpc::Status DeltaVirtualHosts(::grpc::ServerContext* context,
                                     ::grpc::ServerReaderWriter<
                                         ::envoy::service::discovery::v3::DeltaDiscoveryResponse,
                                         ::envoy::service::discovery::v3::DeltaDiscoveryRequest
                                     >* stream) override;
};