#include "routediscoveryserviceimpl.hpp"

using ::envoy::service::discovery::v3::DiscoveryRequest;
using ::envoy::service::discovery::v3::DiscoveryResponse;
using ::envoy::service::discovery::v3::DeltaDiscoveryRequest;
using ::envoy::service::discovery::v3::DeltaDiscoveryResponse;

::grpc::Status RouteDiscoveryServiceImpl::StreamRoutes(
    ::grpc::ServerContext* context,
    ::grpc::ServerReaderWriter<DiscoveryResponse, DiscoveryRequest>* stream)
{
    return ::grpc::Status::OK;
} 
    
::grpc::Status RouteDiscoveryServiceImpl::DeltaRoutes(
    ::grpc::ServerContext* context,
    ::grpc::ServerReaderWriter<DeltaDiscoveryResponse, DeltaDiscoveryRequest>* stream) 
{
    return ::grpc::Status::OK;
} 

::grpc::Status RouteDiscoveryServiceImpl::FetchRoutes(
    ::grpc::ServerContext* context,
    const DiscoveryRequest* request,
    DiscoveryResponse* response)
{
    return ::grpc::Status::OK;
} 