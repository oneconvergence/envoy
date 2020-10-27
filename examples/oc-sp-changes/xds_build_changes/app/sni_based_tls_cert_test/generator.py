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
    'version_info': '0',
    'resources': [
        {
            '@type': 'type.googleapis.com/envoy.config.listener.v3.Listener',
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
    ]
}
FILTER_CHAIN_FILE = """
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
                    'name': 'local_route',
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
                'tls_certificates': [
                    {
                        'certificate_chain': { 'filename': '%(cert)s' },
                        'private_key': { 'filename': '%(key)s' }
                    }
                ]
            }
        }
    }
}
"""

FILTER_CHAIN_ADS = """
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

FILTER_CHAIN_XDS = """
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
                            'resource_api_version': 'V3',
                            'api_config_source': {
                                'api_type': 'DELTA_GRPC',
                                'transport_api_version': 'V3',
                                'grpc_services': {
                                    'envoy_grpc': {
                                        'cluster_name': 'xds_cluster'
                                    }
                                }
                            }
                        }
                    }
                ]
            }
        }
    }
}
"""

FILTER_CHAINS = CONFIG['resources'][0]['filter_chains']


def openssl(*args):
    cmdline = [OPENSSL] + list(args)
    subprocess.check_call(cmdline, stderr=subprocess.DEVNULL)


class CertKeyPairGenerateWorker(Thread):

    def __init__(self, queue, config_source, skip_cert_generation):
        Thread.__init__(self)
        self.queue = queue
        self.skip_cert_generation = skip_cert_generation
        self.config_source = config_source

    def run(self):
        FILTER_CHAIN = None
        if "ads" in self.config_source.lower():
            FILTER_CHAIN = FILTER_CHAIN_ADS
        elif "file" in self.config_source.lower():
            FILTER_CHAIN = FILTER_CHAIN_FILE
        elif "xds" in self.config_source.lower():
            FILTER_CHAIN = FILTER_CHAIN_XDS
        else:
            print("Error: wrong config_source provided for listener config!!! \n"
                  "config-source should be file or ads or xds.")
            return

        while True:
            domain_dict = self.queue.get()
            domain = domain_dict["domain"]
            """subprocess.check_call(['mkdir', CERTS_LOCATION + domain])
            key_path = '%s%s/%s.key' % (CERTS_LOCATION, domain, domain)
            cert_path = '%s%s/%s.crt' % (CERTS_LOCATION, domain, domain)"""
            try:
                print(self.skip_cert_generation)
                if (not self.skip_cert_generation):
                    subprocess.check_call(['mkdir', '-p', CERTS_LOCATION + domain])
                    key_path = '%s%s/%s.key' % (CERTS_LOCATION, domain, domain)
                    cert_path = '%s%s/%s.crt' % (CERTS_LOCATION, domain, domain)
                    openssl('genrsa', '-out', key_path, '2048')
                    openssl('req', '-x509', '-new', '-nodes',
                            '-key', key_path, '-sha256',
                            '-subj', '/C=US/ST=CA/O=Nutanix/CN=%s' % domain,
                            '-days', '365',
                            '-out', cert_path)

                FILTER_CHAINS.append(
                    ast.literal_eval(
                        FILTER_CHAIN
                        % {
                            "domain": domain,
                            "cert": '%s%s/%s.crt' % (CERTS_LOCATION_ENVOY, domain, domain),
                            "key": '%s%s/%s.key' % (CERTS_LOCATION_ENVOY, domain, domain),
                            "route_config_name_suffix": str(random.randint(1,400)) if domain_dict["update"] else ""
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
        help='Start index. Please specify num-certs-to-update along with this option.')
    parser.add_argument(
        '--num',
        type=int,
        default='0', metavar='NUM_CERT_KEY_PAIRS',
        required=True,
        help='Number of certificate and key pairs to generate.')
    parser.add_argument(
        '--num-certs-to-update',
        type=int,
        default='0', metavar='NUM_CERTS_TO_UPDATE',
        required=False,
        help='Number of certificate and key pairs to generate.')
    parser.add_argument(
        '--config-source',
        default='ads', metavar='CONFIG_SOURCE',
        required=True,
        help='Source of envoy config, it can be file or ads or xds')
    parser.add_argument(
        '--skip-cert-generation',
        type=bool,
        default=False, metavar='SKIP_CERTS',
        required=False,
        help='Skip certificate generation, option useful when we have to generate listener config.')
    args = parser.parse_args()
    config_source = args.config_source
    skip_cert_generation = args.skip_cert_generation
    queue = Queue()
    for x in range(WORKERS):
        worker = CertKeyPairGenerateWorker(queue, config_source, skip_cert_generation)
        worker.daemon = True
        worker.start()

    #subprocess.check_call(['mkdir', '-p', CERTS_LOCATION])

    ts = time()

    for y in range(args.start, args.start + args.num_certs_to_update):
        domain = 'service%d.ocenvoy.com' % y
        queue.put({"domain": domain, "update": True})

    for y in range(args.start + args.num_certs_to_update, args.num + args.start):
        domain = 'service%d.ocenvoy.com' % y
        queue.put({"domain": domain, "update": False})

    queue.join()

    
    with open(args.config_file, 'w') as stream:
        if "file" in config_source.lower():
            yaml.dump(CONFIG, stream)
        else:
            del CONFIG['resources'][0]['@type']
            yaml.dump(CONFIG['resources'][0], stream)

    logging.error("%d done in %r secs" % (args.num, (time() - ts)))


if __name__ == '__main__':
    main()
