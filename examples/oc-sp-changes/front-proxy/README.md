To learn about this sandbox and for instructions on how to run it please head over
to the [envoy docs](https://www.envoyproxy.io/docs/envoy/latest/start/sandboxes/front_proxy.html)

---

## To test this sandbox, follow these steps -
1. Generate certs for sds by using following commands and copy them into xds/sds directory as. Copy generator.py in xds directory from [generator_script](https://github.com/oneconvergence/envoy/blob/sp-changes/examples/oc-sp-changes/xds_build_changes/app/sni_based_tls_cert_test/generator.py) and run it as:
```
      $ cd xds
      # copy generator.py here and run it as below to generate 50k certs
      $ python3 generator.py --config listener50k.yaml --start 0 --num 50000 --config-source ads
      $ mv certs/* sds/.
      $ mv listener50k.yaml lds/.
```
2. Build ADS binary using following command from root of this repo i.e envoy_research and update docker-compose.yaml (volume section) to mount it in grpc-server.
```
      $ bazel build --verbose_failures --sandbox_debug -s -c opt -copt="-g" //app/xds_server:xds_server
```

3. Spawn the Docker chain
```
       $ docker-compose build
       $ docker-compose up -d
       # to check status of docker containers
       $ docker-compose ps
```
4. Start envoy process as:
```
       # open new terminal tab and login to envoy docker and start envoy process:
       $ docker-compose exec front-envoy bash
       $ envoy --config-path /etc/front-envoy.yaml
```
5. Start ads-server and configure listener:
```
       # open new terminal tab and login to grpc-server (ads-service container) and start ads-server
       $ docker-compose exec grpc_server bash
       $ cd /opt/ads_server
       $ ./ads_server
       # once ads_server is started, push listener config using ads_server. Type dynamic to enter into cli mode
       dynamic
       Enter the command: add_listener listener1
```    
 above option will configure listener1 with 2 filterchains. Basically ```add_listener listener1``` command loads xds/lds/listener1.yaml config and pushes it to envoy. Similarly you can push listener50k config which we generated in step 1.

6. Test the connection to the service1 based on SNI.
```
       # Add these entries for host resoultion in /etc/hosts
       127.0.0.1 service1.ocenvoy.com
    
       # Make request for service1. The TLS Client HELLO packet should contain the extension 'Server Name' with 'service1.example.com'.
       $ curl --cacert xds/sds/service1.ocenvoy.com/service1.ocenvoy.com.crt https://service1.ocenvoy.com/service/1
    
       # One can run wrk script to check envoy performance, make sure you install wrk before you run below script.
         Check link below to install wrk:
         https://github.com/wg/wrk/wiki/Installing-Wrk-on-Linux
       $ python3 run_wrk_test.py
       # Check result in wrk_result.txt once script execution completes. 
```
7. Destroy the Docker chain (or) end the sandbox

       $ cd front-proxy-sni
       $ docker-compose down

8. ads_server cli also useful in updating the envoy configuration dynamically using option automation. You need to provide listener config [config.yaml](./config.yaml) to automation command. Listener configuration is loaded from xds/lds/ilds directory.
```
   # To generate certs and config for this test you can use generator script as:
   # copy generator.py script to xds directory, then run following commands:
   $ cd xds; mkdir -p lds/ilds
   $ python3 generator.py --config-file listener.yaml.1k --start 0 --num 1000 --config-source ads --skip-cert-generation True
   $ python3 generator.py --config-file listener.yaml.2k --start 0 --num 2000 --config-source ads --skip-cert-generation True
   $ python3 generator.py --config-file listener.yaml.3k --start 0 --num 3000 --config-source ads --skip-cert-generation True
   $ python3 generator.py --config-file listener.yaml.4k --start 0 --num 4000 --config-source ads --skip-cert-generation True
   $ python3 generator.py --config-file listener.yaml.5k --start 0 --num 5000 --config-source ads
   # copy certs in sds directory
   $ mv certs/* sds/.
   # copy listener config to lds/ilds directory
   $ mv listener.yaml* lds/ilds/.

   # To start dynamically update of listener config, use below command in tab where ads_server is running:
   
   dynamic
   Enter the command: automation config.yaml
```
