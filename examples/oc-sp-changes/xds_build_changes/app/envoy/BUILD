package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test"
)

alias(
    name = "envoy",
    actual = ":envoy-static",
)

envoy_cc_binary(
    name = "envoy-static",
    repository = "@envoy",
    deps = [
#        "//app/envoy/filters/http/example:http_filter_config",
        "@envoy//source/exe:envoy_main_entry_lib",
    ],
)
