To learn about this sandbox and for instructions on how to run it please head over
to the [envoy docs](https://www.envoyproxy.io/docs/envoy/latest/start/sandboxes/front_proxy.html)

======================
$ openssl req -nodes -x509 -newkey rsa:2048 -keyout example-com.key -out example-com.crt -days 365
# Provide the string 'localhost' in 'Common Name'

$ curl --cacert example-com.crt --connect-to localhost -H 'Host: example.com' https://localhost/service/1
$ curl --cacert example-com.crt --connect-to localhost -H 'Host: example.com' https://localhost/service/2

$ curl -I -H 'Host: example.com' http://localhost:8000/
