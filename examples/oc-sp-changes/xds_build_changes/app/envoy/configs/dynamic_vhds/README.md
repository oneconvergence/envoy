This is the config files using dynamic file base relaod for envoy. 

The configs are separate into:
   lds.conf - listener config
   eds.conf - end point config
   cds.conf - cluster config

The main config file "envoy_config.yaml" refer to those files in the dynamic_resources sections. 
The *.conf file are in json format which would be the same format when switching to xds.

The vhds section in lds.conf is configured to go to a local xds_server which has to be configure as static_resources in the main envoy_config.yaml

