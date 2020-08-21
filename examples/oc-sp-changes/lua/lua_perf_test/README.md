To learn about this sandbox and for instructions on how to run it please head over
to the [envoy docs](https://www.envoyproxy.io/docs/envoy/latest/start/sandboxes/front_proxy.html)

---

## To test this sandbox, follow these steps -
1. Build envoy binary with HTTP filter, please refer [this link](https://github.com/envoyproxy/envoy-filter-example/tree/master/http-filter-example). Please add "--define exported_symbols=enabled" to bazel build command in above link.

2. Once envoy binary is generated, please copy it in [config_files](./config_files) directory. Also copy [lib](../redis-example/lib) in config_files directory.
```
       $ cp -r ../redis-example/lib config_files/.
```

3. Spawn the Docker chain
```
       $ docker-compose build
       $ docker-compose up -d
       # to check status of docker containers
       $ docker-compose ps
```

4. To run tests, please scale up service containers as:
```
       $ docker-compose scale servicemk1=10
```

5. Start envoy process as:
```
       # open new terminal tab and login to envoy docker and start envoy process:
       $ docker-compose exec front-envoy-mk bash
       $ cd /home/config_files/
       # For redis db tests, start redis server as:
       $ service redis-server start
       $ ./envoy --config-path envoy.yaml
```

6. Configure wrk:
```
       # open new terminal tab and login to wrk docker:
       $ docker-compose exec wrk bash
       $ git clone https://github.com/wg/wrk.git wrk
       $ cd wrk; make
       $ cp wrk /usr/local/bin/.
       # Please update envoy-docker ip address
       $ wrk -t20 -c300 -d300s --timeout 100s --latency http://<envoy-docker-ip>/service/1
       # Above command will run for 5mins.
```

7. Please change envoy config file as per test.
   a. Use [envoy.yaml](./config_files/envoy.yaml) for base configuration.
   b. Use [envoy_hcm.yaml](./config_files/envoy_hcm.yaml) to add headers via HttpConnectionManager filter configuration.
   c. Use [envoy_httpfilter.yaml](./config_files/envoy_httpfilter.yaml) to add headers via custom Http filter configuration.
   d. Use [envoy_lua.yaml](./config_files/envoy_lua.yaml) to add headers via LUA filter configuration.
   e. Use [envoy_redis.yaml](./config_files/envoy_redis.yaml) for read/write keys into redis DB. Please update [testredis.lua](./config_files/lib/testredis.lua) with the redis server ip (local/remote).

8. Destroy the Docker chain (or) end the sandbox

       $ docker-compose down

