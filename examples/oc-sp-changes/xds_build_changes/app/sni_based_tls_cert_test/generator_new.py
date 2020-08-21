import argparse
import ast
import logging
from queue import Queue
import random
import subprocess
import sys
from threading import Thread
from time import time
import yaml

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

WORKERS = 12
OPENSSL = '/usr/local/bin/openssl'
CERTS_LOCATION = './certs/'
CERTS_LOCATION_ENVOY = '/etc/certs/'
CONFIG = {
    'name': 'listener1',
    'address': {
        'socket_address': {'address': '0.0.0.0', 'port_value': 443}
    },
    'listener_filters': [
        {
            'name': 'envoy.filters.listener.tls_inspector',
            'typed_config': {}
        }
    ],
    'filter_chains': []
}
FILTER_CHAIN = """
{
    'filter_chain_match': { 'server_names': [ '%(domain)s' ] },
    'filters': [
        {
            'name': 'envoy.filters.network.http_connection_manager',
            'typed_config': {
                '@type': 'type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager',
                'codec_type': 'AUTO',
                'stat_prefix': 'ingress_http',
                'route_config': {
                    'name': 'local_route%(route_config_name_suffix)s',
                    'virtual_hosts': [
                        {
                            'name': 'backend',
                            'domains': [ '%(domain)s' ],
                            'routes': [
                                {
                                    'match': { 'prefix': '/service/1' },
                                    'route': { 'cluster': 'service1' }
                                }
                            ]
                        }
                    ]
                },
                'http_filters': [
                    {
                        'name': 'envoy.filters.http.router',
                        'typed_config': { }
                    }
                ]
            }
        }
    ],
    'transport_socket': {
        'name': 'envoy.transport_sockets.tls',
        'typed_config': {
            '@type': 'type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.DownstreamTlsContext',
            'common_tls_context': {
                'tls_certificate_sds_secret_configs': [
                    {
                        'name': '%(domain)s',
                        'sds_config': {
                            'ads': {},
                            'resource_api_version': 'V3'
                        }
                    }
                ]
            }
        }
    }
}
"""
FILTER_CHAINS = CONFIG['filter_chains']

"""
                        'sds_config': {
#                            'api_config_source': {
#                                'api_type': 'GRPC',
#                                'transport_api_version': 'V3',
#                                'grpc_services': {
#                                    'envoy_grpc': {
#                                        'cluster_name': 'xds_cluster'
#                                    }
#                                }
#                            },
                            'ads': {},
                            'resource_api_version': 'V3'
                        }

"""


def openssl(*args):
    cmdline = [OPENSSL] + list(args)
    subprocess.check_call(cmdline, stderr=subprocess.DEVNULL)


class CertKeyPairGenerateWorker(Thread):

    def __init__(self, queue):
        Thread.__init__(self)
        self.queue = queue

    def run(self):
        while True:
            domain_dict = self.queue.get()
            domain = domain_dict["domain"]
            #subprocess.check_call(['mkdir', CERTS_LOCATION + domain])
            #key_path = '%s%s/%s.key' % (CERTS_LOCATION, domain, domain)
            #cert_path = '%s%s/%s.crt' % (CERTS_LOCATION, domain, domain)
            try:
                """
                openssl('genrsa', '-out', key_path, '2048')
                openssl('req', '-x509', '-new', '-nodes',
                        '-key', key_path, '-sha256',
                        '-subj', '/C=US/ST=CA/O=Nutanix/CN=%s' % domain,
                        '-days', '365',
                        '-out', cert_path)
                """
                FILTER_CHAINS.append(
                    ast.literal_eval(
                        FILTER_CHAIN
                        % {
                            "domain": domain, "route_config_name_suffix": str(random.randint(1,400)) if domain_dict["update"] else "" 
                        }))
            finally:
                self.queue.task_done()


def main():
    if sys.version_info[0] != 3:
        sys.exit('Execute with python3')

    parser = argparse.ArgumentParser(
        "Generate NUM of certificate chain and private key pairs"
        " and update the configuration YAML.")
    parser.add_argument(
        '--config-file',
        default='', metavar='FILE_PATH',
        required=True,
        help='Listener configuration in YAML file path')
    parser.add_argument(
        '--start',
        type=int,
        default='0', metavar='START_INDEX',
        required=True,
        help='Start index.')
    parser.add_argument(
        '--num',
        type=int,
        default='0', metavar='NUM_CERT_KEY_PAIRS',
        required=True,
        help='Number of certificate and key pairs to generate.')
    args = parser.parse_args()

    queue = Queue()
    for x in range(WORKERS):
        worker = CertKeyPairGenerateWorker(queue)
        worker.daemon = True
        worker.start()

    subprocess.check_call(['mkdir', '-p', CERTS_LOCATION])

    ts = time()

    for y in range(args.start, args.start+200):
        domain = 'service%d.ocenvoy.com' % y
        queue.put({"domain": domain, "update": True})

    for y in range(args.start+200, args.num+args.start):
        domain = 'service%d.ocenvoy.com' % y
        queue.put({"domain": domain, "update": False})

    queue.join()

    with open(args.config_file, 'w') as stream:
        yaml.dump(CONFIG, stream)

    logging.error("%d done in %r secs" % (args.num, (time() - ts)))


if __name__ == '__main__':
    main()
