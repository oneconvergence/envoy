from flask import Flask
from flask import request, send_file
import time
import os
import requests
import socket
import sys

app = Flask(__name__)

TRACE_HEADERS_TO_PROPAGATE = [
    'X-Ot-Span-Context',
    'X-Request-Id',

    # Zipkin headers
    'X-B3-TraceId',
    'X-B3-SpanId',
    'X-B3-ParentSpanId',
    'X-B3-Sampled',
    'X-B3-Flags',

    # Jaeger header (for native client)
    "uber-trace-id"
]

filename = "/code/test.yaml"

@app.route('/service/<service_number>')
def hello(service_number):
  #time.sleep(290)
  return send_file(filename)
'''
@app.route('/service/<service_number>')
def hello(service_number):
  #time.sleep(290)
  data = read_data()
  return ('Hello from behind Envoy (service {})! hostname: {} resolved'
          'hostname: {}\n data: {}'.format(os.environ['SERVICE_NAME'], socket.gethostname(),
                                  socket.gethostbyname(socket.gethostname()), data))
'''

@app.route('/trace/<service_number>')
def trace(service_number):
  headers = {}
  # call service 2 from service 1
  if int(os.environ['SERVICE_NAME']) == 1:
    for header in TRACE_HEADERS_TO_PROPAGATE:
      if header in request.headers:
        headers[header] = request.headers[header]
    ret = requests.get("http://localhost:9000/trace/2", headers=headers)
  #time.sleep(290)
  return ('Hello from behind Envoy (service {})! hostname: {} resolved'
          'hostname: {}\n'.format(os.environ['SERVICE_NAME'], socket.gethostname(),
                                  socket.gethostbyname(socket.gethostname())))

if __name__ == "__main__":
  app.run(host='0.0.0.0', port=8080)
