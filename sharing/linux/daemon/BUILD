load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_binary(
    name = "nearby_sharing_daemon",
    srcs = ["main.cc"],
    deps = [
        ":nearby_sharing_dbus_service",
        "//connections/implementation/flags:connections_flags",
        "//internal/flags:nearby_flags",
        "//sharing:nearby_sharing_service",
        "//sharing/flags/generated:generated_flags",
        "//sharing/linux:linux_sharing_platform",
        "//sharing/proto:enums_cc_proto",
        "@com_google_absl//absl/time",
        "@sdbus_cpp",
    ],
)

cc_library(
    name = "nearby_sharing_dbus_service",
    srcs = ["nearby_sharing_dbus_service.cc"],
    hdrs = [
        "nearby_sharing_dbus_service.h",
        "nearby_sharing_server.h",
    ],
    deps = [
        "//internal/base:file_path",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "@com_google_absl//absl/synchronization",
        "@sdbus_cpp",
    ],
)

cc_test(
    name = "nearby_sharing_dbus_service_test",
    srcs = ["nearby_sharing_dbus_service_test.cc"],
    deps = [
        ":nearby_sharing_dbus_service",
        "//internal/platform/implementation/linux:linux",
        "//sharing:attachments",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "@com_google_googletest//:gtest_main",
        "@sdbus_cpp",
    ],
)
