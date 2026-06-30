load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["notice"])

cc_library(
    name = "sharing_rpc_client",
    hdrs = [
        "identity_rpc_types.h",
        "sharing_rpc_client.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//sharing/proto:share_cc_proto",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "grpc_async_client_factory",
    hdrs = ["grpc_async_client_factory.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":sharing_rpc_client",
        "//location/nearby/sharing/lib/account:account_manager",
        "//sharing/analytics",
        "@com_google_absl//absl/status",
    ],
)

