# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_cc//cc/private/rules_impl:cc_static_library.bzl", "cc_static_library")

cc_library(
    name = "linux_sharing_platform",
    srcs = ["platform/linux_sharing_platform.cc"],
    hdrs = ["platform/linux_sharing_platform.h"],
    visibility = ["//visibility:private"],
    deps = [
        "//internal/base:file_path",
        "//internal/base:files",
        "//internal/platform:base",
        "//internal/platform:mac_address",
        "//internal/platform:types",
        "//internal/platform:uuid",
        "//internal/platform/implementation:account_manager",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation:signin_attempt",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/linux",
        "//sharing/internal/api:platform",
        "//sharing/internal/public:pref_names",
        "//sharing/proto:share_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "nearby_sharing_api",
    srcs = ["nearby_sharing_api.cc"],
    hdrs = ["nearby_sharing_api.h"],
    alwayslink = True,
    visibility = ["//visibility:public"],
    deps = [
        ":linux_sharing_platform",
        "//internal/base:file_path",
        "//internal/platform/implementation/linux:system_clock",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing/analytics",
        "//sharing/local_device_data",
    ],
)

cc_library(
    name = "nearby_connections_api",
    srcs = ["nearby_connections_api.cc"],
    hdrs = ["nearby_connections_api.h"],
    alwayslink = True,
    visibility = ["//visibility:public"],
    deps = [
        ":linux_sharing_platform",
        "//internal/base:file_path",
        "//sharing/internal/public:nearby_context",
        "//sharing:nearby_sharing_service",
    ],
)

cc_static_library(
    name = "nearby_sharing_api_static",
    deps = [":nearby_sharing_api"],
)

cc_binary(
    name = "nearby_sharing_api_shared",
    linkshared = True,
    srcs = [
        "nearby_sharing_api.cc",
        "nearby_sharing_api.h",
    ],
    visibility = ["//visibility:public"],
    linkopts = [
        "-Wl,--exclude-libs,ALL",
    ],
    deps = [
        ":linux_sharing_platform",
        "//internal/base:file_path",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation/linux",
        "//internal/platform/implementation/linux:system_clock",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing/analytics",
        "//sharing/local_device_data",
    ],
)

cc_binary(
    name = "nearby_connections_api_shared",
    linkshared = True,
    srcs = [
        "nearby_connections_api.cc",
        "nearby_connections_api.h",
    ],
    visibility = ["//visibility:public"],
    linkopts = [
        "-Wl,--exclude-libs,ALL",
    ],
    deps = [
        ":linux_sharing_platform",
        "//internal/base:file_path",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation/linux",
        "//sharing/internal/public:nearby_context",
        "//sharing:nearby_sharing_service",
    ],
)

cc_test(
    name = "nearby_connections_api_test",
    srcs = ["nearby_connections_api_test.cc"],
    deps = [
        ":nearby_connections_api",
        "@com_google_googletest//:gtest_main",
    ],
)
