load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
        ":tui" : "",
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
)

cc_binary(
  name = "tui",
  srcs = [
    "main.cc",
  ],
  deps = [
    ":app",
  ]
)

cc_library(
  name = "app",
  srcs = [
    "app.cc",
  ],
  hdrs = [
    "app.h",
  ],
  deps = [
    ":file_picker",
    ":page",
    "//sharing/linux/tui/ui:home_screen",
    "@ftxui//:ftxui",
  ],
)

cc_library(
  name = "page",
  hdrs = [
    "page.h",
  ],
)

cc_library(
  name = "ipc_client",
  srcs = [
    "ipc_client.cc",
  ],
  hdrs = [
    "ipc_client.h",
  ],
  deps = [
    "@nlohmann_json//:json",
  ],
)

cc_test(
  name = "ipc_client_test",
  srcs = [
    "ipc_client_test.cc",
  ],
  deps = [
    ":ipc_client",
    "@com_google_googletest//:gtest_main",
    "@nlohmann_json//:json",
  ],
)

cc_library(
  name = "file_picker",
  srcs = [
    "file_picker.cc",
  ],
  hdrs = [
    "file_picker.h",
  ],
)

cc_library(
  name = "palette",
  hdrs = [
    "palette.h",
  ],
  deps = [
    "@ftxui//:ftxui",
  ]
)
