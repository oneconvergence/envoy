# Envoy filter example

These files are copied from https://github.com/envoyproxy/envoy-filter-example.git 
and modified to compile with our layout as an example.

The main changes needed is:
1. Anywhere it includes the .pb.h file (protobuf generated headers), we need to modify the path to go to:
   ```
   #include "app/envoy/filters/http/<filter_name>/<filter_name>.pb.h"
   ```
