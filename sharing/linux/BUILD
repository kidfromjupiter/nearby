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

cc_library(
    name = "nearby_sharing_service_linux",
    srcs = [
        "nearby_sharing_service_linux.cc",
        "//sharing:nearby_sharing_service.cc",
        "//sharing:transfer_metadata.cc",
        "//sharing:transfer_metadata_builder.cc",
        '//sharing/certificates:nearby_share_certificate_manager.cc',
        '//sharing/certificates:nearby_share_decrypted_public_certificate.cc',
        '//sharing/certificates:nearby_share_encrypted_metadata_key.cc',
        '//sharing/certificates:nearby_share_private_certificate.cc',
        '//sharing/local_device_data:nearby_share_local_device_data_manager.cc',
        '//sharing:nearby_sharing_settings.cc',
        '//sharing/analytics:analytics_recorder.cc',
        '//sharing:thread_timer.cc',
        '//sharing/certificates:common.cc',
        '//sharing/common:nearby_share_prefs.cc',
    ],
    hdrs = [
        "nearby_sharing_service_linux.h",
        "//sharing:transfer_metadata.h",
        "//sharing:transfer_metadata_builder.h",
        '//sharing:nearby_sharing_service.h',
        '//sharing/common:nearby_share_prefs.h',
        '//sharing/certificates:nearby_share_certificate_manager.h',
        '//sharing/certificates:nearby_share_decrypted_public_certificate.h',
        '//sharing/certificates:nearby_share_encrypted_metadata_key.h',
        '//sharing/certificates:nearby_share_private_certificate.h',
        '//sharing/internal/api:private_certificate_data.h',
        '//sharing/local_device_data:nearby_share_local_device_data_manager.h',
        '//sharing:nearby_sharing_settings.h',
        '//sharing/analytics:analytics_recorder.h',
        '//sharing/analytics:analytics_device_settings.h',
        '//sharing/analytics:analytics_information.h',
        '//sharing/internal/api:preference_manager.h',
        '//sharing/internal/public:context.h',
        '//sharing/internal/public:pref_names.h',
        '//sharing/internal/api:bluetooth_adapter.h',
        '//sharing/internal/api:fast_initiation_manager.h',
        '//sharing/internal/api:fast_init_ble_beacon.h',
        '//sharing/internal/public:connectivity_manager.h',
        '//sharing:thread_timer.h',
        '//sharing:share_target_discovered_callback.h',
        '//sharing:transfer_update_callback.h',
        '//sharing/certificates:common.h',
        '//sharing/certificates:constants.h',
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//connections:core",
        "//connections:core_types",
        "//internal/platform:base",
        "//internal/platform:comm",
        "//internal/platform:types",
        "//internal/platform/implementation:platform",
        "//internal/platform/implementation/linux:linux",
        "//sharing:attachments",
        "//sharing:types",
        "//sharing/proto:share_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "nearby_sharing_app",
    srcs = ["nearby_sharing_app.cc"],
    deps = [
        ":nearby_sharing_service_linux",
        "//sharing:attachments",
        "//sharing:types",
        "//internal/platform:base",
        "@com_google_absl//absl/strings",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "simple_example",
    srcs = ["simple_example.cc"],
    deps = [
        ":nearby_sharing_service_linux",
        "//sharing:attachments",
        "//sharing:types",
        "//internal/platform:base",
        "@com_google_absl//absl/strings",
    ],
    visibility = ["//visibility:public"],
)
