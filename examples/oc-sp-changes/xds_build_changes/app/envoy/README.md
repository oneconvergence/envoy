To add a new http filter:
1. Create a directory under filters/http/ (look at filters/http/example/BUILD to setup the BUILD file)
2. Put you filter implementation code, config code and config proto file in there
3. Add the dependency to the top BUILD file in the envoy_cc_binary section under this directory. For example, add the xyz_filter like below:
```
envoy_cc_binary(
    name = "envoy-static",
    repository = "@envoy",
    deps = [
        "//app/envoy/filters/http/xyv:xyz_filter_config",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
        "//app/envoy/filters/http/example:http_filter_config",
```

Build ADS as:
1. Build envoy for vhds config via ADS. Run following command from root of repo (envoy_research):
```
CC=clang bazel build --verbose_failures --sandbox_debug -s -c opt --copt="-g" //app/envoy:envoy
```
2. Build ADS server as:
```
CC=clang bazel build --verbose_failures --sandbox_debug -s -c opt --copt="-g" //app/ads_server:ads_server
```
