load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_qt//:qt.bzl", "qt_cc_binary", "qt_cc_library", "qt_resource_via_qrc")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",

    # Specify the targets of interest.
    # For example, specify a dict of targets and any flags required to build.
    targets = {
        ":app" : "",
    },
    # No need to add flags already in .bazelrc. They're automatically picked up.
    # If you don't need flags, a list of targets is also okay, as is a single target string.
    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
    # And if you're working on a header-only library, specify a test or binary target that compiles it.
)

qt_resource_via_qrc(
    name = "app_resources",
    qrc_file = "resources.qrc",
    files = [
        "main.qml",
        "qmldir",
        "AppContent.qml",
        "EventBus.qml",
        "Drop.qml",
        "Sidebar.qml",
        "ShareTarget.qml",
        "SearchingDevices.qml",
        "Targets.qml",
        "IncomingShare.qml",
        "TransferCard.qml",
        "googlesans_var.ttf",
        "icons/file.svg",
        "icons/laptop.svg",
        "icons/quickshare.svg",
        "icons/smartphone.svg",
        "icons/tablet.svg",
        "icons/up_file.svg",
    ],
)

qt_cc_library(
    name = "backend",
    srcs = ["backend.cc"],
    hdrs = ["backend.h"],
    deps = [
        "//connections/implementation/flags:connections_flags",
        "//internal/base:file_path",
        "//internal/flags:nearby_flags",
        "//sharing:attachments",
        "//sharing:nearby_sharing_service",
        "//sharing:transfer_metadata",
        "//sharing:types",
        "//sharing/flags/generated:generated_flags",
        "//sharing/linux:linux_sharing_platform",
        "//sharing/proto:enums_cc_proto",
        "@com_google_absl//absl/time",
        "@rules_qt//:qt_core",
        "@rules_qt//:qt_hdrs",
        "@rules_qt//:qt_qml",
        "@rules_qt//:qt_qml_meta",
    ],
)

cc_library(
    name = "native_file_logging",
    srcs = ["native_file_logging.cc"],
    hdrs = ["native_file_logging.h"],
    deps = [
        "//sharing/linux:linux_sharing_platform",
        "@com_google_absl//absl/base:log_severity",
        "@com_google_absl//absl/log:globals",
        "@com_google_absl//absl/log:initialize",
        "@com_google_absl//absl/log:log_entry",
        "@com_google_absl//absl/log:log_sink",
        "@com_google_absl//absl/log:log_sink_registry",
        "@com_google_absl//absl/strings:string_view",
    ],
)

cc_test(
    name = "native_file_logging_test",
    size = "small",
    srcs = ["native_file_logging_test.cc"],
    deps = [
        ":native_file_logging",
        "//sharing/linux:linux_sharing_platform",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:globals",
        "@com_google_absl//absl/log:initialize",
        "@com_google_absl//absl/log:log_sink",
        "@com_google_absl//absl/log:log_sink_registry",
        "@com_google_googletest//:gtest_main",
    ],
)

qt_cc_binary(
    name = "app",
    srcs = ["main.cc"],
    env = {
        "QT_QPA_PLATFORM": "wayland;xcb",
    },
    deps = [
        ":app_resources",
        ":backend",
        ":native_file_logging",
        "@rules_qt//:qt_core",
        "@rules_qt//:qt_gui",
        "@rules_qt//:qt_hdrs",
        "@rules_qt//:qt_labs_platform",
        "@rules_qt//:qt_opengl",
        "@rules_qt//:qt_qml",
        "@rules_qt//:qt_qml_meta",
        "@rules_qt//:qt_qml_workerscript",
        "@rules_qt//:qt_quick",
        "@rules_qt//:qt_quick_controls2",
        "@rules_qt//:qt_quick_layouts",
        "@rules_qt//:qt_quick_shapes",
        "@rules_qt//:qt_widgets",
    ],
)

sh_binary(
    name = "appimage",
    srcs = ["packaging/package_minimal_appimage.sh"],
    data = [
        ":app",
        "icons/quickshare.svg",
        "packaging/AppRun",
        "packaging/quickshare.desktop",
        "packaging/quickshare.metainfo.xml",
        "@appimage_runtime_x86_64//file",
        "@appimagetool_x86_64//file",
    ],
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)
