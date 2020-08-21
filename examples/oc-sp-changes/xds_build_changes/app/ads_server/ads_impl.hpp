#include <grpc/grpc.h>

#include "envoy/service/discovery/v3/ads.grpc.pb.h" // AggregatedDiscoveryService
#include "envoy/config/listener/v3/listener.pb.h" // Listener
#include "envoy/config/listener/v3/listener.pb.validate.h" // Listener validator
#include "envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.pb.h" // HttpConnectionManager
#include "envoy/extensions/access_loggers/file/v3/file.pb.validate.h" // FileAccessLog validator
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h" // Secret
#include "envoy/config/route/v3/route.pb.h" // RouteConfiguration and Vhds
#include "envoy/config/route/v3/route.pb.validate.h" // RouteConfiguration validator
#include "envoy/config/route/v3/route_components.pb.h" // VirtualHost, Route etc.
#include "envoy/config/route/v3/route_components.pb.validate.h" // VirtualHost and Route validator

#include "resource.hpp"
#include "util.hpp"

void SendADSResponseSotW(std::string type_url);
void SendADSResponseIncremental(std::string type_url,
                                std::set<std::string>* delta_resources,
                                std::set<std::string>* removed_resources);

class AggregatedDiscoveryServiceImpl final : public envoy::service::discovery::v3::AggregatedDiscoveryService::Service {
public:
    grpc::Status StreamAggregatedResources(grpc::ServerContext* context,
                                           grpc::ServerReaderWriter<
                                               envoy::service::discovery::v3::DiscoveryResponse,
                                               envoy::service::discovery::v3::DiscoveryRequest
                                           >* stream) override;
    
    grpc::Status DeltaAggregatedResources(grpc::ServerContext* context,
                                          grpc::ServerReaderWriter<
                                              envoy::service::discovery::v3::DeltaDiscoveryResponse,
                                              envoy::service::discovery::v3::DeltaDiscoveryRequest
                                          >* stream) override;
};
