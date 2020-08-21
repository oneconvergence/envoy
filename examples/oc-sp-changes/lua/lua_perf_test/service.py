from flask import Flask
from flask import request, send_file
import time
import os
import socket
import sys

app = Flask(__name__)


filename = "/code/test.bin"

@app.route('/service/<service_number>')
def hello(service_number):
  #time.sleep(290)
  return send_file(filename)


if __name__ == "__main__":
  app.run(host='0.0.0.0', port=8080)
