load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
  name = "icons",
  srcs = [
    "file_icon.cc",
    "brand.cc",
    "laptop_icon.cc",
    "phone_icon.cc",
    "tablet_icon.cc",
  ],
  hdrs = [
    "file_icon.h",
    "brand.h",
    "laptop_icon.h",
    "phone_icon.h",
    "tablet_icon.h",
  ],
  deps = [
    "@ftxui//:ftxui",
  ],
)

cc_library(
  name = "share_target",
  srcs = [
    "share_target.cc",
  ],
  hdrs = [
    "share_target.h",
  ],
  deps = [
    "@ftxui//:ftxui",
    "//sharing/linux/tui:palette",
    ":icons",
  ],
)
cc_library(
  name = "file_picker_card",
  srcs = [
    "file_picker_card.cc",
  ],
  hdrs = [
    "file_picker_card.h",
  ],
  deps = [
    ":icons",
    "//sharing/linux/tui:file_picker",
    "//sharing/linux/tui:palette",
    "@ftxui//:ftxui",
  ],
)
