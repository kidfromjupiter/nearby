load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_binary(
    name = "nearby_sharing_daemon",
    srcs = ["main.cc"],
    deps = [
        ":daemon_service",
        ":ipc_server",
        "//connections/implementation/flags:connections_flags",
        "//internal/flags:nearby_flags",
        "//sharing:nearby_sharing_service",
        "//sharing/flags/generated:generated_flags",
        "//sharing/linux:linux_sharing_platform",
        "//sharing/proto:enums_cc_proto",
        "@com_google_absl//absl/time",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "daemon_service",
    srcs = ["daemon_service.cc"],
    hdrs = ["daemon_service.h"],
    deps = [
        "//internal/base:file_path",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@nlohmann_json//:json",
    ],
)

cc_test(
    name = "daemon_service_test",
    srcs = ["daemon_service_test.cc"],
    deps = [
        ":daemon_service",
        "//internal/platform/implementation/linux:linux",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "@com_google_googletest//:gtest_main",
        "@nlohmann_json//:json",
    ],
)

cc_test(
    name = "ipc_server_test",
    srcs = ["ipc_server_test.cc"],
    deps = [
        ":ipc_server",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/synchronization",
        "@nlohmann_json//:json",
    ],
)

cc_library(
  name = "ipc_server",
  hdrs = ["ipc_server.h"],
  srcs = ["ipc_server.cc"],
  deps = [
        "@com_google_absl//absl/synchronization",
        "@nlohmann_json//:json",
  ],
)
