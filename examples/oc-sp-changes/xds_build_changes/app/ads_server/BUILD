package(default_visibility = ["//visibility:public"])

load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

# grpc route_guide example. Left here for reference example and can be removed later
#grpc_proto_library(
#    name = "route_guide",
#    srcs = ["route_guide.proto"],
#)
#

#cc_binary(
#   name = "route_guide_server",
#    srcs = [
#        "helper.cc",
#        "helper.h",
#        "route_guide_server.cc",
#    ],
#    data = ["route_guide_db.json"],
#    defines = ["BAZEL_BUILD"],
#    deps = [
#        "//protos:route_guide",
#        "@com_github_grpc_grpc//:grpc++",
#    ],
#)

cc_binary(
    name = "ads_server",
    copts = [
      "-std=c++14", # Envoy needs this
    ],
    linkopts = [
      "-lstdc++fs", # For std::experimental::filesystem
    ],
    srcs = [
        "ads_server.cpp",
        "ads_impl.cpp",
        "ads_impl.hpp",
        "resource.hpp",
        "resource.cpp",
        "util.cpp",
        "util.hpp",
    ],
    deps = [
        "@envoy_api//envoy/service/discovery/v3:pkg_cc_grpc", # ADS - ads.proto
        "@envoy_api//envoy/extensions/transport_sockets/tls/v3:pkg_cc_proto", # SDS - cert.proto
        "@envoy_api//envoy/config/listener/v3:pkg_cc_proto", # LDS - listener.proto
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto", # LDS - http_connection_manager.proto
        "@envoy_api//envoy/extensions/access_loggers/file/v3:pkg_cc_proto", # LDS - access_logger - file.proto
        "@envoy_api//envoy/config/route/v3:pkg_cc_proto", # RDS & VHDS - rds.proto
        "@envoy//source/common/protobuf:utility_lib", # common/protobuf/utility.h
        "@envoy//source/common/protobuf:message_validator_lib", # common/protobuf/message_validator_impl.h
        "@envoy//source/common/config:version_converter_lib", # common/config/version_converter.h
        "@envoy//include/envoy/common:base_includes", # envoy/common/exception.h
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_fmtlib_fmt//:fmtlib",
        "//external:yaml_cpp",
        "//external:spdlog",
    ],
)
