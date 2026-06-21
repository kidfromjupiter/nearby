load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
  name = "file_icon",
  srcs = [
    "file_icon.cc",
  ],
  hdrs = [
    "file_icon.h",
  ],
  deps = [
    "@ftxui//:ftxui",
  ],
)

cc_library(
  name = "brand",
  srcs = [
    "brand.cc",
  ],
  hdrs = [
    "brand.h",
  ],
  deps = [
    "@ftxui//:ftxui",
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
    ":file_icon",
    "//sharing/linux/tui:file_picker",
    "//sharing/linux/tui:palette",
    "@ftxui//:ftxui",
  ],
)
