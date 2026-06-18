load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["notice"])

cc_library(
    name = "sync_binding_prefs_cc_proto",
    hdrs = ["sync_binding_prefs.pb.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sync_manager",
    hdrs = ["sync_manager.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":sync_binding_prefs_cc_proto",
        "//internal/base:file_path",
        "//location/nearby/sharing/lib/rpc:sharing_rpc_client",
        "//sharing/internal/api:platform",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:string_view",
    ],
)

