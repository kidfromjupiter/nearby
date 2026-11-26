# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

licenses(["notice"])

cc_library(
    name = "command_line",
    srcs = ["command_line.cc"],
    hdrs = ["command_line.h"],
    visibility = ["//visibility:private"],
)

cc_binary(
    name = "nearby_connections_cli",
    srcs = ["main.cc"],
    deps = [
        ":command_line",
        "//connections:core",
        "//connections:core_types",
        "//internal/crypto_cros:crypto_cros",
        "//internal/platform/implementation/linux:linux",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "command_line_test",
    size = "small",
    srcs = ["command_line_test.cc"],
    deps = [
        ":command_line",
        "@com_google_googletest//:gtest_main",
    ],
)
