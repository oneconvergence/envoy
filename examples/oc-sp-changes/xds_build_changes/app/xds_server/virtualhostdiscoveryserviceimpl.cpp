#include "virtualhostdiscoveryserviceimpl.hpp"

using ::envoy::service::discovery::v3::DeltaDiscoveryRequest;
using ::envoy::service::discovery::v3::DeltaDiscoveryResponse;

::grpc::Status VirtualHostDiscoveryServiceImpl::DeltaVirtualHosts(::grpc::ServerContext* context,
                                                                  ::grpc::ServerReaderWriter<DeltaDiscoveryResponse, DeltaDiscoveryRequest>* stream)
{
    return ::grpc::Status::OK;
}
