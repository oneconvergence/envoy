# LUA- Example to access redis from lua script

* Install and start redis server using following command:
  apt-get install -y redis-server
  service redis-server start

* Configure redis-server ip and port in ./lib/testconstants.lua file.
  In testconstants.lua file, we are accessing redis-server to fetch value by key.
  e.g by using client:get("host") we are retriving value of key "host".

* Install lua dependencies on the host/container where envoy will run:
  apt-get install -y luarocks
  luarocks install luasocket

* Install python flask if not installed on host/container where backend server (../service.py)
  will be running, and configure it as upstream cluser for envoy.
  pip install flask
  python ../service.py  

* Make sure envoy is built with "--define exported_symbols=enabled" for bazel build command to load lua ext libraries.

* Start envoy as (running with single worker):
  envoy --config-path envoy_redis.yaml -l debug --concurrency 1

* Now send curl request from same host/container where envoy is running:
  curl -v http://127.0.0.1/hello/1

* In response we can see following 2 headers:
```
curl -v http://127.0.0.1/hello/1
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 80 (#0)
> GET /hello/1 HTTP/1.1
> Host: 127.0.0.1
> User-Agent: curl/7.58.0
> Accept: */*
>
< HTTP/1.1 200 OK
< content-length: 1024
< content-type: application/octet-stream
< last-modified: Thu, 04 Jun 2020 05:59:36 GMT
< cache-control: public, max-age=43200
< expires: Tue, 28 Jul 2020 19:18:31 GMT
< etag: "1591250376.0-1024-721814939"
< server: envoy
< date: Tue, 28 Aug 2020 07:18:30 GMT
< x-envoy-upstream-service-time: 2
< modified-host: stackpath.com
< modified-port: 1234
< request-count: 1
<
* Closing connection 0
```

