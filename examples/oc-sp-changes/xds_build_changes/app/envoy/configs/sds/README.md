To quickly test this configuration,

 1. Replace the following files in the `front-proxy` example with the one in this directory.

 - `front-envoy.yaml`
 - `docker-compose.yaml`

   > [NOTE]
   > 
   > In the front-envoy.yaml name the resource names that needs to be subscribed through sds as FQDNs, Ex: service1.ocenvoy.com. And for validation_context the resource name must be trusted_ca
   > 
   >       transport_socket:
   >         name: envoy.transport_sockets.tls
   >         typed_config:
   >           "@type": type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.DownstreamTlsContext
   >           common_tls_context:
   >             tls_certificate_sds_secret_configs:
   >               - name: service1.ocenvoy.com
   >               sds_config:
   >                 api_config_source:
   >                   api_type: GRPC
   >                   transport_api_version: V3
   >                   grpc_services:
   >                     envoy_grpc:
   >                       cluster_name: sds_server_mtls
   >                 resource_api_version: V3
   >             validation_context_sds_secret_config:
   >               name: trusted_ca
   >               sds_config:
   >                 api_config_source:
   >                   api_type: DELTA_GRPC
   >                   transport_api_version: V3
   >                   grpc_services:
   >                     envoy_grpc:
   >                       cluster_name: sds_server_mtls
   >                 resource_api_version: V3
   >
   > 
   > Mount the following volumes for grpc_server
   > a) `directory.path.containing.the.xds_server.binary` at `/opt/xds_server`
   > b) `directory.path.containing.certificates.per.service` at `/opt/xds_server/secret`
   >  
   >  Example structure for  `directory.path.containing.certificates.per.service`,
   >  
   >       $ tree directory.path.containing.certificates.per.service
   >       directory.path.containing.certificates.per.service
   >       ├── ca.crt
   >       ├── service0.ocenvoy.com
   >       │   ├── service0.ocenvoy.com.crt
   >       │   └── service0.ocenvoy.com.key
   >       ├── service1.ocenvoy.com
   >       │   ├── service1.ocenvoy.com.crt
   >       │   └── service1.ocenvoy.com.key
   >       ├── service2.ocenvoy.com
   >       │   ├── service2.ocenvoy.com.crt
   >       │   └── service2.ocenvoy.com.key
   >        ...
   >       └── servicen.ocenvoy.com
   >           ├── servicen.ocenvoy.com.crt
   >           └── servicen.ocenvoy.com.key`

 2. Add the following files in the `front-proxy` example from this directory.

 - `Dockerfile-GRPC_Server`

 3. Create a directory `certs` inside the `front-proxy` example directory.

 4. Generate the server and client certificates for mTLS connection to XDS server. Also copy these files to the destination where `xds_server` binary is available.

        $ cd front-proxy/certs/
        $ openssl req -nodes -x509 -newkey rsa:2048 -keyout client.key -out client.crt -days 365 -subj "/C=US/O=Test/OU=Server/CN=client"
        $ openssl req -nodes -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -subj "/C=US/O=Test/OU=Server/CN=server"

