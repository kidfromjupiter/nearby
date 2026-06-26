load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

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

# 1. Compile QML files into a C++ source file using Qt's Resource Compiler (rcc)
genrule(
    name = "qrc_generation",
    srcs = [
        "resources.qrc",
        "main.qml",
        "Drop.qml",
        "Sidebar.qml",
        "googlesans_var.ttf",
        "icons/file.svg",
        "icons/up_file.svg"
    ],
    outs = ["qrc_resources.cpp"],
    cmd = "/usr/lib64/qt6/libexec/rcc $(location resources.qrc) -o $(location qrc_resources.cpp)",
)

# 2. MOC generation for backend.h
genrule(
    name = "moc_backend",
    srcs = ["backend.h"],
    outs = ["moc_backend.cpp"],
    cmd = "/usr/lib64/qt6/libexec/moc $(location backend.h) -o $(location moc_backend.cpp) ",
)

cc_binary(
    name = "app",
    srcs = [
        "backend.cc",
        "main.cc",
        "backend.h",
        ":qrc_generation",  # Include the generated QRC file
        ":moc_backend",
    ],
    copts = [
        # Base Qt6 directory
        "-I/usr/include/qt6",
        # Individual module directories required for QGuiApplication and QQmlApplicationEngine
        "-I/usr/include/qt6/QtCore",
        "-I/usr/include/qt6/QtGui",
        "-I/usr/include/qt6/QtQml",
        "-I/usr/include/qt6/QtQuick",
    ],
    linkopts = [
        "-lQt6Core",
        "-lQt6Gui",
        "-lQt6Qml",
        "-lQt6Quick",
    ],
    deps = [
        ":nearby_sharing_dbus_client",
    ],
)

cc_library(
    name = "nearby_sharing_dbus_client",
    srcs = ["nearby_sharing_dbus_client.cc"],
    hdrs = ["nearby_sharing_dbus_client.h"],
    deps = [
        "//sharing/linux/daemon:nearby_sharing_dbus_client_glue",
        "@sdbus_cpp",
    ],
)
