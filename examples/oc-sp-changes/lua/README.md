# Sample lua filter example
## To setup please follow below steps:

1. Install lua and lua libraries to compile sample cpp code (ER-13),
```
On ubuntu:
$ sudo apt-get install lua5.1 liblua5.1-dev
$ g++ -Wall -shared -fPIC -o power.so -I/usr/include/lua5.1 -llua5.1 hellofunc.cpp

On centos:
$ yum install lua5.1.4 lua-devel-5.1.4
$ g++ -Wall -shared -fPIC -o power.so -I/usr/include/ hellofunc.cpp
```
2. Now run envoy as:
```
   envoy --config-path ./envoy.yaml --log-level debug
```
3. Send curl request with service/1 as path:
```
curl -v http://127.0.0.1:80/service/1
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 80 (#0)
> GET /service/1 HTTP/1.1
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
< date: Tue, 28 Jul 2020 07:18:30 GMT
< x-envoy-upstream-service-time: 2
< response-body-size: 1024
< square: 9
< 
* Closing connection 0
```
Note that response-body-size (ext lua lib) and square (ext cpp so lib) headers are added through lua filter.

4. Send curl request with stackpath as path:
```
curl -v http://127.0.0.1:80/stackpath/1
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 80 (#0)
> GET /stackpath/1 HTTP/1.1
> Host: 127.0.0.1
> User-Agent: curl/7.58.0
> Accept: */*
> 
< HTTP/1.1 403 Forbidden
< content-length: 4
< response-body-size: 4
< square: 9
< date: Tue, 28 Jul 2020 07:22:41 GMT
< server: envoy
< 
* Connection #0 to host 127.0.0.1 left intact
```

Here url matches stackpath pattern and lua sends direct response (403)


# Example-2 (ER-14 & ER-15)
* Install flask
  ```
  pip install flask
  ``` 
* Start server
  ```
  # Create 10M file to send it in chunks
  dd if=/dev/zero of=dummyfile.bin bs=1024 count=10240
  python server.py
  ```
* Start envoy with single worked thread to demonstrate ER-14 in another terminal
  ```
  envoy --config-path ./front-envoy-lua.yaml -l debug --concurrency 1
  ```
* Send first request to access 10M file (body is sent in chunks)
  ```
  curl --http2 http://127.0.0.1/service/1 --output /dev/null
  # this will take some time to complete
  ```
* Send another request from another terminal
  ```
  curl --http2 http://127.0.0.1:8080/service/1
  ```
