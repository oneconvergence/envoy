This is the experimental meta-repo for the CDN4 C++ development

When adding a new repo or changing branch, run:
   $ ./build_tools/scm/generate_branch_manifest.sh

and commit the branch.manifest file


To build the envoy static binary, run:
```
bazel build --verbose_failures --sandbox_debug -s -c opt --copt="-g" //app/envoy:envoy
```
To build the xds_server, run:
```
bazel build --verbose_failures --sandbox_debug -s -c opt -copt="-g" //app/xds_server:xds_server
```
To build the ads_server, run:
```
bazel build --verbose_failures --sandbox_debug -s -c opt -copt="-g" //app/ads_server:ads_server
```

