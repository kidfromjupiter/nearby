load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["notice"])

cc_library(
    name = "rpc",
    hdrs = [
        "grpc_async_client_factory.h",
        "identity_rpc_types.h",
        "sharing_rpc_client.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//internal/platform/implementation:account_manager",
        "//sharing/analytics",
        "//sharing/proto:share_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "sync",
    hdrs = ["sync_manager.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//sharing/internal/api:platform",
        "@com_google_absl//absl/strings:string_view",
    ],
)

cc_library(
    name = "highway_fingerprint",
    hdrs = ["highway_fingerprint.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/strings:string_view",
    ],
)
