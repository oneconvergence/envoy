For OC developers, use the oc_develop branch as the submodules in other branches are pointing to SP internal bitbucket:
```
  git clone https://andy-fong@bitbucket.org/stackpath/envoy_research.git
  cd envoy_research/
  git checkout -b oc_develop origin/oc_develop
  git submodule update --init --recursive --remote
```

Example filter is in app/envoy/filters/http/example. To compile with the example filter, uncomment the example:http_filter_config line in app/envoy/BUILD

To build the envoy static binary, run:
```
bazel build --verbose_failures --sandbox_debug -s -c opt --copt="-g" //app/envoy:envoy
```
To build the xds_server, run:
```
bazel build --verbose_failures --sandbox_debug -s -c opt -copt="-g" //app/xds_server:xds_server
```

