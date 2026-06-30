load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(

)

cc_test(
    name = "linux_connections_test",
    size = "small",
    srcs = [
        "atomic_boolean_test.cc",
        "atomic_reference_test.cc",
        "mutex_test.cc",
        "utils_test.cc",
    ],
    tags = ["notap"],
    deps = [
        "//internal/platform/implementation/linux:comm",
        "//internal/platform/implementation/linux:crypto",
        "//internal/platform/implementation/linux:linux",
        "//internal/platform/implementation/linux:test_utils",
        "//internal/platform/implementation/linux:types",
        "//internal/platform:base",
        "//internal/platform/implementation:comm",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/shared:count_down_latch",
        "@com_github_protobuf_matchers//protobuf-matchers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
        "@nlohmann_json//:json",
    ],
)
