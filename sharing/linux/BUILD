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

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
        ":fast_init": "",
        ":nearby_connections_manager": "",
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
)


cc_library(
    name = "fast_init",
    hdrs = [
        "nearby_fast_init_ble_beacon.h",
        "nearby_fast_init_manager.h",
    ],
    srcs = [ 
        "nearby_fast_init_manager.cc",
    ],
    deps = [
        "//sharing/internal/api:platform",
        "//connections:core",
        "//internal/platform/implementation/linux"
    ]
)


cc_binary(
    name = "fast_init_test",
    srcs = ["nearby_fast_init.cc"],
    deps = [":fast_init"],
)

cc_binary(
    name = "nearby_connections",
    srcs = [
        "nearby_connections.cc",
    ],
    deps = [
      ":nearby_connections_manager",
      ":fast_init",
      "//connections:core",
      "//internal/platform/implementation/linux:linux"
    ]
)

cc_binary(
    name = "nearby_sharing_cli",
    srcs = ["nearby_sharing_cli.cc"],
    deps = [
        ":linux_sharing_platform",
        "//connections/implementation/flags:connections_flags",
        "//internal/base:file_path",
        "//internal/flags:nearby_flags",
        "//proto:sharing_enums_cc_proto",
        "//sharing:attachments",
        "//sharing/flags/generated:generated_flags",
        "//sharing:nearby_sharing_service",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "//sharing/analytics",
        "//sharing/common:enum",
        "//sharing/proto:enums_cc_proto",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "linux_sharing_platform",
    srcs = [
        "platform/linux_account_manager.cc",
        "platform/linux_account_manager.h",
        "platform/linux_platform_components.cc",
        "platform/linux_platform_components.h",
        "platform/linux_preference_manager.cc",
        "platform/linux_preference_manager.h",
        "platform/linux_sharing_platform.cc",
        "platform/platform_util.cc",
        "platform/platform_util.h",
    ],
    hdrs = ["platform/linux_sharing_platform.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":fast_init",
        "//internal/base:file_path",
        "//internal/base:files",
        "//internal/platform:base",
        "//internal/platform:logging",
        "//internal/platform:mac_address",
        "//internal/platform:types",
        "//internal/platform:uuid",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/linux",
        "//location/nearby/sharing/lib/account:account_manager",
        "//location/nearby/sharing/lib/sync:sync_binding_prefs_cc_proto",
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
