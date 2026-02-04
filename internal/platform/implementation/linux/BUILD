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

#load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

licenses(["notice"])

#refresh_compile_commands(
#    name = "refresh_compile_commands",
#
#    # Specify the targets of interest.
#    # For example, specify a dict of targets and any flags required to build.
#    targets = {
#        ":linux": "-s --check_visibility=false --spawn_strategy=standalone --verbose_failures --strip=never --copt=-O0 --copt=-g --copt=-fno-omit-frame-pointer",
#        "//connections:core": "-s --check_visibility=false --spawn_strategy=standalone --verbose_failures --strip=never --copt=-O0 --copt=-g --copt=-fno-omit-frame-pointer",
#        "//connections/file_share:file_share": "-s --check_visibility=false --spawn_strategy=standalone --verbose_failures --strip=never --copt=-O0 --copt=-g --copt=-fno-omit-frame-pointer",
#    },
#    # No need to add flags already in .bazelrc. They're automatically picked up.
#    # If you don't need flags, a list of targets is also okay, as is a single target string.
#    # Wildcard patterns, like //... for everything, *are* allowed here, just like a build.
#    # As are additional targets (+) and subtractions (-), like in bazel query https://docs.bazel.build/versions/main/query.html#expressions
#    # And if you're working on a header-only library, specify a test or binary target that compiles it.
#)

cc_library(
    name = "types",
    srcs = [
        "device_info.cc",
        #"log_message.cc",
        "timer.cc",
    ],
    hdrs = [
        "atomic_boolean.h",
        "atomic_reference.h",
        "atomic_uint32.h",
        "bluetooth_adapter.h",
        "condition_variable.h",
        "device_info.h",
        "executor.h",
        "future.h",
        "mutex.h",
        "preferences_manager.h",
        "preferences_repository.h",
        "scheduled_executor.h",
        "submittable_executor.h",
        "timer.h",
        "thread_pool.h",
        #"log_message.h",
        "utils.h",
    ],
    copts = ["-lrt"],
    visibility = ["//third_party/nearby/sharing/internal/impl/linux:__pkg__"],
    deps = [
        ":comm",
        "//internal/platform/implementation:types",
        "@com_google_absl//absl/strings",
        "@sdbus_cpp",
        "@sdbus_cpp//:libsystemd",
    ],
)

cc_library(
    name = "comm",
    hdrs = [
        "avahi.h",
        "ble_gatt_server.h",
        "ble_gatt_client.h",
        #        "ble_medium.h",
        "ble_v2_medium.h",
        "ble_v2_server_socket.h",
        "ble_v2_socket.h",
        "ble_l2cap_server_socket.h",
        "ble_l2cap_socket.h",
        "bluetooth_adapter.h",
        "bluetooth_bluez_profile.h",
        "bluetooth_classic_device.h",
        "bluetooth_classic_medium.h",
        "bluetooth_classic_server_socket.h",
        "bluetooth_classic_socket.h",
        "bluetooth_devices.h",
        "bluetooth_pairing.h",
        "bluez.h",
        "bluez_device.h",
        #        "bluez_agent.h",
        "bluez_advertisement_monitor.h",
        "bluez_advertisement_monitor_manager.h",
        "bluez_gatt_characteristic_client.h",
        "bluez_gatt_characteristic_server.h",
        "bluez_gatt_manager.h",
        "bluez_gatt_profile.h",
        "bluez_gatt_service_client.h",
        "bluez_gatt_service_server.h",
        "bluez_le_advertisement.h",
        "dbus.h",
        "network_manager.h",
        "network_manager_active_connection.h",
        "network_manager_access_point.h",
        "stream.h",
        "tcp_server_socket.h",
        #        "wifi_direct.h",
        #        "wifi_direct_server_socket.h",
        #        "wifi_direct_socket.h",
        "wifi_hotspot.h",
        "wifi_hotspot_server_socket.h",
        "wifi_hotspot_socket.h",
        "wifi_lan.h",
        "wifi_lan_server_socket.h",
        "wifi_lan_socket.h",
        "wifi_medium.h",
        "wifi_socket.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":crypto",
        "//internal/platform:base",
        "//internal/platform:comm",
        "//internal/platform:types",
        "//internal/platform:uuid",
        "//internal/platform/implementation:comm",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/linux/generated:types",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:optional",
        "@sdbus_cpp",
        "@sdbus_cpp//:libsystemd",
    ],
)

cc_library(
    name = "crypto",
    srcs = [
        "crypto.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//internal/platform:base",
        "//internal/platform/implementation:types",
        "@boringssl//:crypto",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "linux",
    srcs = [
        "avahi.cc",
        "ble_gatt_client.cc",
        "ble_gatt_server.cc",
        #        "ble_medium.cc",
        "ble_v2_medium.cc",
        "ble_v2_server_socket.cc",
        "ble_v2_socket.cc",
        "ble_l2cap_server_socket.cc",
        "ble_l2cap_socket.cc",
        "bluetooth_adapter.cc",
        "bluetooth_bluez_profile.cc",
        "bluetooth_classic_socket.cc",
        "bluetooth_classic_device.cc",
        "bluetooth_classic_medium.cc",
        "bluetooth_classic_server_socket.cc",
        "bluetooth_devices.cc",
        "bluetooth_pairing.cc",
        "bluez.cc",
        #"bluez_agent.cc",
        "bluez_advertisement_monitor.cc",
        "bluez_gatt_characteristic_client.cc",
        "bluez_gatt_characteristic_server.cc",
        "bluez_gatt_service_server.cc",
        "bluez_le_advertisement.cc",
        "dbus.cc",
        "executor.cc",
        "network_manager.cc",
        "network_manager_active_connection.cc",
        "platform.cc",
        "preferences_manager.cc",
        "preferences_repository.cc",
        "scheduled_executor.cc",
        "stream.cc",
        "submittable_executor.cc",
        "system_clock.cc",
        "thread_pool.cc",
        "utils.cc",
        #        "wifi_direct.cc",
        #        "wifi_direct_server_socket.cc",
        "wifi_hotspot.cc",
        "wifi_hotspot_server_socket.cc",
        "wifi_lan.cc",
        "wifi_lan_server_socket.cc",
        "wifi_medium.cc",
    ],
    linkopts = [
        "-lcurl",
        "-lbluetooth",
    ],
    visibility = [
        "//connections:__subpackages__",
        "//fastpair:__subpackages__",
        "//location/nearby:__subpackages__",
        "//presence:__subpackages__",
        "//third_party/nearby/sharing:__subpackages__",
    ],
    deps = [
        ":comm",
        ":crypto",  # build_cleaner: keep
        ":types",
        "//internal/flags:nearby_flags",
        "//internal/platform:base",
        "//internal/platform:cancellation_flag",
        "//internal/platform:comm",
        "//internal/platform:types",
        "//internal/platform:uuid",
        "//internal/platform/flags:platform_flags",
        "//internal/platform/implementation:comm",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/linux/generated:types",
        "//internal/platform/implementation/shared:count_down_latch",
        "//internal/platform/implementation/shared:file",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:optional",
        "@nlohmann_json//:json",
        "@sdbus_cpp",
        "@sdbus_cpp//:libsystemd",
    ],
)

cc_library(
    name = "test_utils",
    srcs = [
        "test_utils.cc",
    ],
    hdrs = [
        "test_data.h",
        "test_utils.h",
    ],
    visibility = [
        "//visibility:private",  # Only private by automation, not intent. Owner may accept CLs adding visibility. See go/scheuklappen#explicit-private.
    ],
    deps = [
        ":types",
        "//internal/platform:base",
        "@nlohmann_json//:json",
    ],
)

cc_test(
    name = "impl_test",
    size = "small",
    srcs = [
        "atomic_boolean_test.cc",
        "atomic_reference_test.cc",
        "mutex_test.cc",
        "utils_test.cc",
        # "bluetooth_adapter_test.cc",
        # "crypto_test.cc",
        # "device_info_test.cc",
        # "executor_test.cc",
        # "file_path_test.cc",
        # "http_loader_test.cc",
        # "preferences_manager_test.cc",
        # "preferences_repository_test.cc",
        # "scheduled_executor_test.cc",
        # "submittable_executor_test.cc",
        # "thread_pool_test.cc",
        # "timer_test.cc",
    ],
    tags = ["notap"],
    deps = [
        ":comm",
        ":crypto",
        ":linux",
        ":test_utils",
        ":types",
        "//internal/platform:base",
        "//internal/platform/implementation:comm",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation:types",
        "//internal/platform/implementation/shared:count_down_latch",
        "@com_github_protobuf_matchers//protobuf-matchers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
        "@nlohmann_json//:json",
    ],
)
