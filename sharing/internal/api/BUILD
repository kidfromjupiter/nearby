# Copyright 2023 Google LLC
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

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary")

licenses(["notice"])

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands_sharing_platform",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
        ":platform": "",
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
)

cc_library(
    name = "platform",
    hdrs = [
        "app_info.h",
        "bluetooth_adapter.h",
        "fast_init_ble_beacon.h",
        "fast_initiation_manager.h",
        "network_monitor.h",
        "preference_manager.h",
        "private_certificate_data.h",
        "public_certificate_database.h",
        "sharing_platform.h",
        "system_info.h",
    ],
    visibility = [
        "//internal/account:__pkg__",
        "//location/nearby/analytics/cpp/logging:__pkg__",
        "//location/nearby/cpp/sharing:__subpackages__",
        "//location/nearby/sharing/lib:__subpackages__",
        "//location/nearby/sharing/sdk/test_client:__pkg__",
        "//sharing:__subpackages__",
    ],
    deps = [
        "//internal/base:file_path",
        "//internal/platform:mac_address",
        "//internal/platform:types",
        "//internal/platform/implementation:types",
        "//location/nearby/sharing/lib/account:account_manager",
        "//location/nearby/sharing/lib/sync:sync_binding_prefs_cc_proto",
        "//sharing/proto:share_cc_proto",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "mock_sharing_platform",
    testonly = True,
    hdrs = [
        "mock_app_info.h",
        "mock_bluetooth_adapter.h",
        "mock_fast_init_ble_beacon.h",
        "mock_fast_initiation_manager.h",
        "mock_network_monitor.h",
        "mock_public_certificate_db.h",
        "mock_sharing_platform.h",
        "mock_system_info.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":platform",
        "//internal/base:file_path",
        "//internal/platform:mac_address",
        "//internal/platform:types",
        "//internal/platform/implementation:types",
        "//location/nearby/sharing/lib/account:account_manager",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_for_library_testonly",
    ],
)
