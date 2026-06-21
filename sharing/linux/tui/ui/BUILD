load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
  name = "home_header",
  srcs = [
    "home_header.cc",
  ],
  hdrs = [
    "home_header.h",
  ],
  deps = [
    "@ftxui//:ftxui",
    "//sharing/linux/tui/components:icons"
  ],
)

cc_library(
  name = "sidebar",
  srcs = [
    "sidebar.cc",
  ],
  hdrs = [
    "sidebar.h",
  ],
  deps = [
    "//sharing/linux/tui:palette",
    "//sharing/linux/tui/components:icons",
    "@ftxui//:ftxui",
  ],
)

cc_library(
  name = "file_selected_screen",
  srcs = [
    "file_selected_screen.cc",
  ],
  hdrs = [
    "file_selected_screen.h",
  ],
  deps = [
    "//sharing/linux/tui:palette",
    "//sharing/linux/tui/components:share_target",
    "@ftxui//:ftxui",
  ],
)

cc_library(
  name = "home_screen",
  srcs = [
    "home_screen.cc",
  ],
  hdrs = [
    "home_screen.h",
  ],
  deps = [
    ":file_selected_screen",
    ":home_header",
    ":sidebar",
    "//sharing/linux/tui:file_picker",
    "//sharing/linux/tui:page",
    "//sharing/linux/tui:palette",
    "//sharing/linux/tui/components:file_picker_card",
    "@ftxui//:ftxui",
  ],
)
