load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "ipc_server_test",
    srcs = ["ipc_server_test.cc"],
    deps = [
        ":ipc_server",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_library(
  name = "ipc_server",
  hdrs = ["ipc_server.h"],
  deps = [
        "@com_google_absl//absl/synchronization",
  ],
)
