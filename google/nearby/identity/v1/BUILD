load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["notice"])

cc_library(
    name = "resources_cc_proto",
    hdrs = ["resources.pb.h"],
    visibility = ["//visibility:public"],
    deps = ["//location/nearby/sharing/lib/rpc:sharing_rpc_client"],
)

cc_library(
    name = "rpcs_cc_proto",
    hdrs = ["rpcs.pb.h"],
    visibility = ["//visibility:public"],
    deps = [":resources_cc_proto"],
)

