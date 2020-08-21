from flask import Flask, Response
from flask import request, send_file
import time
import os
import requests
import socket
import sys

app = Flask(__name__)


filename = "dummyfile.bin"

@app.route('/hello/<service_number>')
def testhello(service_number):
  return ('Hello from backend server! hostname: {} resolved'
          'hostname: {}\n'.format( socket.gethostname(),
                                  socket.gethostbyname(socket.gethostname())))

@app.route('/service/<service_number>')
def hello(service_number):
  #time.sleep(290)
  #return send_file(filename)
  def generate():
    with open(filename, "br") as f:
      chunk_size = 1048576 # 1m
      while True:
        chunk = f.read(chunk_size)
        if not chunk:
          break
        time.sleep(2)
        yield chunk
  return Response(generate(), mimetype='application/octet-stream')

if __name__ == "__main__":
  app.run(debug=True, host='0.0.0.0', port=8888)
