# Copyright 2024 Google LLC
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
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

licenses(["notice"])

refresh_compile_commands(
    name = "refresh_compile_commands_fast_initiation",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
        ":nearby_fast_initiation": "",
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
)
cc_library(
    name = "nearby_fast_initiation",
    srcs = [
        "nearby_fast_initiation_impl.cc",
    ],
    hdrs = [
        "nearby_fast_initiation.h",
        "nearby_fast_initiation_impl.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//internal/base",
        "//sharing/internal/api:platform",
        "//sharing/internal/public:logging",
        "//sharing/internal/public:types",
    ],
)

cc_library(
    name = "test_support",
    testonly = True,
    srcs = [
        "fake_nearby_fast_initiation.cc",
    ],
    hdrs = [
        "fake_nearby_fast_initiation.h",
        "fake_nearby_fast_initiation_observer.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":nearby_fast_initiation",
        "//internal/base",
        "//sharing/internal/public:logging",
        "//sharing/internal/public:types",
        "@com_google_absl//absl/memory",
    ],
)

cc_test(
    name = "nearby_fast_initiation_test",
    srcs = [
        "nearby_fast_initiation_impl_test.cc",
    ],
    deps = [
        ":nearby_fast_initiation",
        ":test_support",
        "//internal/platform/implementation:platform_impl",
        "//sharing/internal/api:platform",
        "//sharing/internal/test:nearby_test",
        "@com_github_protobuf_matchers//protobuf-matchers",
        "@com_google_googletest//:gtest_main",
    ],
)
