To learn about this sandbox and for instructions on how to run it please head over
to the [envoy docs](https://www.envoyproxy.io/docs/envoy/latest/start/sandboxes/front_proxy.html)


=============================
$ openssl req -nodes -x509 -newkey rsa:2048 -keyout service1-example-com.key -out service1-example-com.crt -days 365
# Provide the string 'service1.example.com' in 'Common Name'
$ openssl req -nodes -x509 -newkey rsa:2048 -keyout service2-example-com.key -out service2-example-com.crt -days 365
# Provide the string 'service2.example.com' in 'Common Name'

$ curl --cacert service1-example-com.crt --resolve service1.example.com:443:0.0.0.0 https://service1.example.com/service/1
$ curl --cacert service2-example-com.crt --resolve service2.example.com:443:0.0.0.0 https://service2.example.com/service/2

$ curl -I -H 'Host: example.com' http://localhost:8000/
