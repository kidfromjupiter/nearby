// swift-tools-version:5.7
// The swift-tools-version declares the minimum version of Swift required to build this package.

// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import PackageDescription

let package = Package(
  name: "NearbyConnections",
  platforms: [
    .iOS(.v13),
    .macOS(.v10_15),
    .tvOS(.v11),
    .watchOS(.v4),
  ],
  products: [
    // Products define the executables and libraries a package produces, and make them visible to other packages.
    .library(
      name: "NearbyConnections",
      targets: ["NearbyConnections"]
    ),
    .library(
      name: "NearbyConnectionsDynamic",
      type: .dynamic,
      targets: ["NearbyConnections"]
    ),
  ],
  dependencies: [
    // Dependencies declare other packages that this package depends on.
    .package(
      url: "https://github.com/bourdakos1/abseil-cpp-SwiftPM.git",
      branch: "cxx17-test"
    ),
    .package(
      url: "https://github.com/firebase/boringssl-SwiftPM.git",
      "0.7.1"..<"0.8.0"
    ),
  ],
  targets: [
    // Targets are the basic building blocks of a package. A target can define a module or a test suite.
    // Targets can depend on other targets in this package, and on products in packages this package depends on.
    .target(
      name: "google-toolbox-for-mac",
      path: "third_party/google-toolbox-for-mac",
      exclude: [
        "google-toolbox-for-mac/Foundation/GTMNSString+HTMLTest.m",
        "google-toolbox-for-mac/Foundation/GTMSystemVersionTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSFileHandle+UniqueName.m",
        "google-toolbox-for-mac/Foundation/GTMLoggerRingBufferWriterTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSData+zlib.m",
        "google-toolbox-for-mac/Foundation/GTMNSNumber+64BitTest.m",
        "google-toolbox-for-mac/Foundation/GTMScriptRunnerTest.m",
        "google-toolbox-for-mac/Foundation/GTMSystemVersion.m",
        "google-toolbox-for-mac/Foundation/GTMFileSystemKQueueTest.m",
        "google-toolbox-for-mac/Foundation/GTMFileSystemKQueue.m",
        "google-toolbox-for-mac/Foundation/GTMGeometryUtilsTest.m",
        "google-toolbox-for-mac/Foundation/GTMLocalizedStringTest.m",
        "google-toolbox-for-mac/Foundation/BUILD",
        "google-toolbox-for-mac/Foundation/GTMSQLite.m",
        "google-toolbox-for-mac/Foundation/GTMLoggerRingBufferWriter.m",
        "google-toolbox-for-mac/Foundation/GTMStringEncoding.m",
        "google-toolbox-for-mac/Foundation/GTMNSFileHandle+UniqueNameTest.m",
        "google-toolbox-for-mac/Foundation/GTMLogger+ASLTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSObject+KeyValueObservingTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+XMLTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSThread+BlocksTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+HTML.m",
        "google-toolbox-for-mac/Foundation/GTMSignalHandlerTest.m",
        "google-toolbox-for-mac/Foundation/GTMURLBuilderTest.m",
        "google-toolbox-for-mac/Foundation/GTMLoggerTest.m",
        "google-toolbox-for-mac/Foundation/GTMServiceManagementTestingHarness.c",
        "google-toolbox-for-mac/Foundation/GTMSQLiteTest.m",
        "google-toolbox-for-mac/Foundation/GTMTimeUtilsTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSFileManager+PathTest.m",
        "google-toolbox-for-mac/Foundation/GTMServiceManagement.c",
        "google-toolbox-for-mac/Foundation/GTMNSString+URLArgumentsTest.m",
        "google-toolbox-for-mac/Foundation/GTMRegexTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSThread+Blocks.m",
        "google-toolbox-for-mac/Foundation/GTMNSScanner+JSONTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSData+zlibTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSDictionary+URLArgumentsTest.m",
        "google-toolbox-for-mac/Foundation/GTMLogger+ASL.m",
        "google-toolbox-for-mac/Foundation/GTMServiceManagementTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+URLArguments.m",
        "google-toolbox-for-mac/Foundation/GTMNSNumber+64Bit.m",
        "google-toolbox-for-mac/Foundation/GTMNSScanner+JSON.m",
        "google-toolbox-for-mac/Foundation/GTMGeometryUtils.m",
        "google-toolbox-for-mac/Foundation/GTMRegex.m",
        "google-toolbox-for-mac/Foundation/GTMScriptRunner.m",
        "google-toolbox-for-mac/Foundation/GTMStackTrace.m",
        "google-toolbox-for-mac/Foundation/GTMURLBuilder.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+FindFolderTest.m",
        "google-toolbox-for-mac/Foundation/GTMObjC2Runtime.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+FindFolder.m",
        "google-toolbox-for-mac/Foundation/GTMNSDictionary+URLArguments.m",
        "google-toolbox-for-mac/Foundation/GTMStackTraceTest.m",
        "google-toolbox-for-mac/Foundation/GTMLightweightProxyTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSFileManager+Path.m",
        "google-toolbox-for-mac/Foundation/GTMStringEncodingTest.m",
        "google-toolbox-for-mac/Foundation/GTMNSObject+KeyValueObserving.m",
        "google-toolbox-for-mac/Foundation/GTMTimeUtils.m",
        "google-toolbox-for-mac/Foundation/GTMSignalHandler.m",
        "google-toolbox-for-mac/Foundation/GTMNSString+XML.m",
        "google-toolbox-for-mac/Foundation/GTMLightweightProxy.m",
        "google-toolbox-for-mac/AppKit",
        "google-toolbox-for-mac/DebugUtils",
        "google-toolbox-for-mac/LICENSE",
        "google-toolbox-for-mac/WORKSPACE",
        "google-toolbox-for-mac/BUILD",
        "google-toolbox-for-mac/GTMiPhone-Info.plist",
        "google-toolbox-for-mac/README.md",
        "google-toolbox-for-mac/XcodeConfig",
        "google-toolbox-for-mac/BuildingAndUsing.txt",
        "google-toolbox-for-mac/GTM-Info.plist",
        "google-toolbox-for-mac/GTMiPhone.xcodeproj",
        "google-toolbox-for-mac/UnitTest-Info.plist",
        "google-toolbox-for-mac/bazel_support",
        "google-toolbox-for-mac/CONTRIBUTING.md",
        "google-toolbox-for-mac/GTM.xcodeproj",
        "google-toolbox-for-mac/GoogleToolboxForMac.podspec",
        "google-toolbox-for-mac/UnitTesting",
        "google-toolbox-for-mac/iPhone",
      ],
      sources: [
        "google-toolbox-for-mac/Foundation/GTMLogger.m"
      ],
      publicHeadersPath: "include",
      cSettings: [
        .headerSearchPath("google-toolbox-for-mac/"),
        .unsafeFlags(["-fno-objc-arc"]),
      ]
    ),
    .target(
      name: "smhasher",
      path: "third_party/smhasher",
      exclude: [
        "include",
        "smhasher/src/CMakeLists.txt",
        "smhasher/src/main.cpp",
        "smhasher/README.md",
        "smhasher/src/AvalancheTest.cpp",
        "smhasher/src/Bitslice.cpp",
        "smhasher/src/Bitvec.cpp",
        "smhasher/src/CityTest.cpp",
        "smhasher/src/City.cpp",
        "smhasher/src/crc.cpp",
        "smhasher/src/DifferentialTest.cpp",
        "smhasher/src/Hashes.cpp",
        "smhasher/src/KeysetTest.cpp",
        "smhasher/src/lookup3.cpp",
        "smhasher/src/md5.cpp",
        "smhasher/src/MurmurHash1.cpp",
        "smhasher/src/MurmurHash2.cpp",
        "smhasher/src/Platform.cpp",
        "smhasher/src/Random.cpp",
        "smhasher/src/sha1.cpp",
        "smhasher/src/SpeedTest.cpp",
        "smhasher/src/Spooky.cpp",
        "smhasher/src/SpookyTest.cpp",
        "smhasher/src/Stats.cpp",
        "smhasher/src/SuperFastHash.cpp",
        "smhasher/src/Types.cpp",
        "smhasher/src/PMurHash.c",
      ],
      sources: [
        "smhasher/src/MurmurHash3.cpp"
      ],
      publicHeadersPath: "include",
      cSettings: [
        .headerSearchPath("smhasher/src/")
      ]
    ),
    .target(
      name: "ukey2",
      dependencies: [
        "protobuf",
        .product(name: "AbseilCXX17", package: "abseil-cpp-SwiftPM"),
        .product(
          name: "openssl_grpc", package: "boringssl-SwiftPM",
          moduleAliases: ["NearbySSL": "openssl_grpc"]),
      ],
      path: "third_party/ukey2",
      exclude: [
        "ukey2/src/main/cpp/src/securegcm/ukey2_shell.cc",
        "ukey2/src/main/cpp/test",
        "ukey2/src/main/java",
        "ukey2/src/main/javatest",
        "ukey2/src/main/proto",
        "ukey2/third_party",
        "ukey2/Android.bp",
        "ukey2/build.gradle",
        "ukey2/CONTRIBUTING.md",
        "ukey2/LICENSE",
        "ukey2/MODULE_LICENSE_APACHE2",
        "ukey2/NOTICE",
        "ukey2/README",
        "ukey2/README.md",
      ],
      sources: [
        "ukey2/src/securemessage/src/securemessage",
        "ukey2/src/main/cpp/src/securegcm",
        "compiled_proto",
      ],
      publicHeadersPath: "include",
      cSettings: [
        .headerSearchPath("ukey2/"),
        .headerSearchPath("compiled_proto/"),
        .headerSearchPath("compiled_proto/src/main/"),
      ]
    ),
    .target(
      name: "json",
      path: "third_party/json",
      exclude: [
        "json/LICENSES",
        "json/cmake",
        "json/docs",
        "json/single_include",
        "json/tests",
        "json/tools",
        "json/CITATION.cff",
        "json/CMakeLists.txt",
        "json/ChangeLog.md",
        "json/LICENSE.MIT",
        "json/Makefile",
        "json/README.md",
        "json/meson.build",
        "json/nlohmann_json.natvis",
        "json/wsjcpp.yml",
      ],
      sources: ["EmptySwiftPackageManagerFile.cpp"],
      publicHeadersPath: "json/include"
    ),
    .target(
      name: "protobuf",
      path: "third_party/protobuf/src",
      exclude: [
        "solaris",
        "libprotoc.map",
        "libprotobuf-lite.map",
        "Makefile.am",
        "libprotobuf.map",
        "README.md",
        "google/protobuf/compiler",
        "google/protobuf/testdata",
        "google/protobuf/testing",
        "google/protobuf/util",
        "google/protobuf/unittest_import_public_lite.proto",
        "google/protobuf/unittest_import.proto",
        "google/protobuf/unittest_lite_imports_nonlite.proto",
        "google/protobuf/no_field_presence_test.cc",
        "google/protobuf/unittest_lazy_dependencies_custom_option.proto",
        "google/protobuf/unittest_well_known_types.proto",
        "google/protobuf/arenastring_unittest.cc",
        "google/protobuf/descriptor.proto",
        "google/protobuf/reflection_ops_unittest.cc",
        "google/protobuf/stubs/stringprintf_unittest.cc",
        "google/protobuf/empty.proto",
        "google/protobuf/map_lite_unittest.proto",
        "google/protobuf/test_util_lite.cc",
        "google/protobuf/any_test.cc",
        "google/protobuf/stubs/structurally_valid_unittest.cc",
        "google/protobuf/unittest_proto3_arena_lite.proto",
        "google/protobuf/stubs/strutil_unittest.cc",
        "google/protobuf/unittest_preserve_unknown_enum.proto",
        "google/protobuf/arena_test_util.cc",
        "google/protobuf/any_test.proto",
        "google/protobuf/timestamp.proto",
        "google/protobuf/io/tokenizer_unittest.cc",
        "google/protobuf/test_util.cc",
        "google/protobuf/map_test.cc",
        "google/protobuf/drop_unknown_fields_test.cc",
        "google/protobuf/descriptor_database_unittest.cc",
        "google/protobuf/unittest_no_generic_services.proto",
        "google/protobuf/unittest_mset_wire_format.proto",
        "google/protobuf/map_unittest.proto",
        "google/protobuf/unittest_no_field_presence.proto",
        "google/protobuf/unittest_proto3_arena.proto",
        "google/protobuf/stubs/stringpiece_unittest.cc",
        "google/protobuf/stubs/time_test.cc",
        "google/protobuf/unittest_optimize_for.proto",
        "google/protobuf/map_test_util.inc",
        "google/protobuf/stubs/statusor_test.cc",
        "google/protobuf/wire_format_unittest.inc",
        "google/protobuf/repeated_field_reflection_unittest.cc",
        "google/protobuf/stubs/template_util_unittest.cc",
        "google/protobuf/unittest_import_public.proto",
        "google/protobuf/io/printer_unittest.cc",
        "google/protobuf/test_messages_proto3.proto",
        "google/protobuf/io/zero_copy_stream_unittest.cc",
        "google/protobuf/text_format_unittest.cc",
        "google/protobuf/stubs/int128_unittest.cc",
        "google/protobuf/unittest_lazy_dependencies_enum.proto",
        "google/protobuf/test_messages_proto2.proto",
        "google/protobuf/unittest_arena.proto",
        "google/protobuf/unittest_lite.proto",
        "google/protobuf/type.proto",
        "google/protobuf/test_util.inc",
        "google/protobuf/extension_set_unittest.cc",
        "google/protobuf/preserve_unknown_enum_test.cc",
        "google/protobuf/unittest_lazy_dependencies.proto",
        "google/protobuf/io/io_win32_unittest.cc",
        "google/protobuf/unittest_embed_optimize_for.proto",
        "google/protobuf/lite_unittest.cc",
        "google/protobuf/unknown_field_set_unittest.cc",
        "google/protobuf/stubs/bytestream_unittest.cc",
        "google/protobuf/wire_format_unittest.cc",
        "google/protobuf/unittest_proto3_lite.proto",
        "google/protobuf/api.proto",
        "google/protobuf/stubs/status_test.cc",
        "google/protobuf/generated_message_reflection_unittest.cc",
        "google/protobuf/unittest_custom_options.proto",
        "google/protobuf/map_field_test.cc",
        "google/protobuf/field_mask.proto",
        "google/protobuf/proto3_arena_lite_unittest.cc",
        "google/protobuf/lite_arena_unittest.cc",
        "google/protobuf/repeated_field_unittest.cc",
        "google/protobuf/unittest_mset.proto",
        "google/protobuf/duration.proto",
        "google/protobuf/proto3_arena_unittest.cc",
        "google/protobuf/unittest_import_lite.proto",
        "google/protobuf/inlined_string_field_unittest.cc",
        "google/protobuf/io/coded_stream_unittest.cc",
        "google/protobuf/unittest_drop_unknown_fields.proto",
        "google/protobuf/source_context.proto",
        "google/protobuf/generated_message_tctable_impl.inc",
        "google/protobuf/map_proto2_unittest.proto",
        "google/protobuf/proto3_lite_unittest.cc",
        "google/protobuf/descriptor_unittest.cc",
        "google/protobuf/proto3_lite_unittest.inc",
        "google/protobuf/wrappers.proto",
        "google/protobuf/any.proto",
        "google/protobuf/stubs/common_unittest.cc",
        "google/protobuf/arena_unittest.cc",
        "google/protobuf/unittest_enormous_descriptor.proto",
        "google/protobuf/map_lite_test_util.cc",
        "google/protobuf/unittest_proto3.proto",
        "google/protobuf/struct.proto",
        "google/protobuf/unittest_preserve_unknown_enum2.proto",
        "google/protobuf/io/gzip_stream_unittest.sh",
        "google/protobuf/unittest_empty.proto",
        "google/protobuf/port_def.inc",
        "google/protobuf/dynamic_message_unittest.cc",
        "google/protobuf/unittest_proto3_optional.proto",
        "google/protobuf/port_undef.inc",
        "google/protobuf/message_unittest.cc",
        "google/protobuf/message_unittest.inc",
        "google/protobuf/unittest.proto",
        "google/protobuf/well_known_types_unittest.cc",
        "google/protobuf/map_test.inc",
        "google/protobuf/reflection_tester.cc",
      ],
      sources: [
        "google/protobuf"
      ],
      publicHeadersPath: ".",
      cSettings: [
        .headerSearchPath("./")
      ]
    ),
    .target(
      name: "NearbyCoreAdapter",
      dependencies: [
        "google-toolbox-for-mac",
        "json",
        "smhasher",
        "ukey2",
        "protobuf",
        .product(name: "AbseilCXX17", package: "abseil-cpp-SwiftPM"),
      ],
      path: ".",
      exclude: [
        "compiled_proto/connections/cpp",
        "presence",
        "embedded",
        "connections/c",
        "connections/connectionsd",
        "connections/dart",
        "connections/java",
        "connections/clients/ios",
        "connections/README.md",
        "connections/swift/NearbyConnections",
        "connections/swift/NearbyCoreAdapter/BUILD",
        "connections/swift/NearbyCoreAdapter/Tests",
        "internal/platform/implementation/g3",
        "internal/platform/implementation/android",
        "internal/platform/implementation/apple/Tests",
        "internal/platform/implementation/apple/Mediums/Ble/Sockets/Tests",
        "internal/platform/implementation/linux",
        "internal/platform/implementation/windows",
        "third_party",
        "CONTRIBUTING.md",
        "LICENSE",
        "README.md",
        "WORKSPACE",
        // build files
        "connections/implementation/analytics/BUILD",
        "connections/implementation/flags/BUILD",
        "connections/implementation/mediums/ble_v2/BUILD",
        "connections/implementation/mediums/multiplex/BUILD",
        "connections/implementation/mediums/BUILD",
        "connections/implementation/BUILD",
        "connections/implementation/fuzzers",
        "connections/v3/BUILD",
        "connections/BUILD",
        "internal/crypto/BUILD",
        "internal/crypto/BUILD.gn",
        "internal/crypto_cros/BUILD",
        "internal/interop/BUILD",
        "internal/weave/BUILD",
        "internal/weave/sockets/BUILD",
        "internal/platform/flags/BUILD",
        "internal/platform/implementation/shared/BUILD",
        "internal/platform/implementation/apple/Mediums/BUILD",
        "internal/platform/implementation/apple/Mediums/Ble/Sockets/BUILD",
        "internal/platform/implementation/apple/Tests/BUILD",
        "internal/platform/implementation/apple/BUILD",
        "internal/platform/implementation/BUILD",
        "internal/platform/BUILD",
        "internal/analytics/BUILD",
        "internal/flags/BUILD",
        "internal/network/BUILD",
        "internal/base/BUILD",
        "internal/data/BUILD",
        "internal/test/BUILD",
        // tests
        "connections/listeners_test.cc",
        "connections/strategy_test.cc",
        "connections/implementation/offline_frames_test.cc",
        "connections/implementation/offline_service_controller_test.cc",
        "connections/implementation/encryption_runner_test.cc",
        "connections/implementation/p2p_cluster_pcp_handler_test.cc",
        "connections/implementation/p2p_point_to_point_pcp_handler_test.cc",
        "connections/implementation/base_pcp_handler_test.cc",
        "connections/implementation/injected_bluetooth_device_store_test.cc",
        "connections/implementation/internal_payload_factory_test.cc",
        "connections/implementation/client_proxy_test.cc",
        "connections/implementation/payload_manager_test.cc",
        "connections/implementation/offline_frames_validator_test.cc",
        "connections/implementation/service_controller_router_test.cc",
        "connections/implementation/bluetooth_bwu_test.cc",
        "connections/implementation/wifi_direct_bwu_test.cc",
        "connections/implementation/wifi_hotspot_bwu_test.cc",
        "connections/implementation/analytics/analytics_recorder_test.cc",
        "connections/implementation/analytics/throughput_recorder_test.cc",
        "connections/implementation/mediums/ble_v2_test.cc",
        "connections/implementation/mediums/ble_v2/bloom_filter_test.cc",
        "connections/implementation/mediums/ble_v2/ble_packet_test.cc",
        "connections/implementation/mediums/ble_v2/ble_advertisement_test.cc",
        "connections/implementation/mediums/ble_v2/advertisement_read_result_test.cc",
        "connections/implementation/mediums/ble_v2/ble_advertisement_header_test.cc",
        "connections/implementation/mediums/ble_v2/ble_utils_test.cc",
        "connections/implementation/mediums/ble_v2/discovered_peripheral_tracker_test.cc",
        "connections/implementation/mediums/ble_v2/instant_on_lost_advertisement_test.cc",
        "connections/implementation/mediums/ble_v2/instant_on_lost_manager_test.cc",
        "connections/implementation/mediums/multiplex/multiplex_frames_test.cc",
        "connections/implementation/mediums/multiplex/multiplex_socket_test.cc",
        "connections/implementation/mediums/multiplex/multiplex_output_stream_test.cc",
        "connections/implementation/mediums/webrtc_peer_id_test.cc",
        "connections/implementation/mediums/wifi_lan_test.cc",
        "connections/implementation/mediums/bluetooth_classic_test.cc",
        "connections/implementation/mediums/ble_test.cc",
        "connections/implementation/mediums/webrtc_test.cc",
        "connections/implementation/mediums/lost_entity_tracker_test.cc",
        "connections/implementation/mediums/bluetooth_radio_test.cc",
        "connections/implementation/mediums/wifi_direct_test.cc",
        "connections/implementation/mediums/wifi_hotspot_test.cc",
        "connections/implementation/mediums/wifi_test.cc",
        "connections/implementation/endpoint_channel_manager_test.cc",
        "connections/implementation/bwu_manager_test.cc",
        "connections/implementation/base_bwu_handler_test.cc",
        "connections/implementation/endpoint_manager_test.cc",
        "connections/implementation/bluetooth_device_name_test.cc",
        "connections/implementation/wifi_lan_service_info_test.cc",
        "connections/implementation/pcp_manager_test.cc",
        "connections/implementation/ble_advertisement_test.cc",
        "connections/implementation/base_endpoint_channel_test.cc",
        "connections/implementation/reconnect_manager_test.cc",
        "connections/v3/connections_device_test.cc",
        "connections/v3/connections_device_provider_test.cc",
        "connections/implementation/connections_authentication_transport_test.cc",
        "connections/core_test.cc",
        "connections/status_test.cc",
        "connections/payload_test.cc",
        "internal/base/bluetooth_address_test.cc",
        "internal/base/files_test.cc",
        "internal/crypto/ed25519_unittest.cc",
        "internal/crypto_cros/aead_unittest.cc",
        "internal/crypto_cros/ec_private_key_unittest.cc",
        "internal/crypto_cros/ec_signature_creator_unittest.cc",
        "internal/crypto_cros/encryptor_unittest.cc",
        "internal/crypto_cros/hmac_unittest.cc",
        "internal/crypto_cros/rsa_private_key_unittest.cc",
        "internal/crypto_cros/secure_hash_unittest.cc",
        "internal/crypto_cros/sha2_unittest.cc",
        "internal/crypto_cros/signature_verifier_unittest.cc",
        "internal/crypto_cros/symmetric_key_unittest.cc",
        "internal/data/leveldb_data_set_test.cc",
        "internal/data/memory_data_set_test.cc",
        "internal/flags/nearby_flags_test.cc",
        "internal/proto/analytics/connections_log_test.cc",
        "internal/platform/feature_flags_test.cc",
        "internal/platform/cancelable_alarm_test.cc",
        "internal/platform/crypto_test.cc",
        "internal/platform/byte_array_test.cc",
        "internal/platform/bluetooth_utils_test.cc",
        "internal/platform/credential_storage_impl_test.cc",
        "internal/platform/input_stream_test.cc",
        "internal/platform/single_thread_executor_test.cc",
        "internal/platform/scheduled_executor_test.cc",
        "internal/platform/count_down_latch_test.cc",
        "internal/platform/pipe_test.cc",
        "internal/platform/timer_impl_test.cc",
        "internal/platform/task_runner_impl_test.cc",
        "internal/platform/uuid_test.cc",
        "internal/platform/wifi_lan_connection_info_test.cc",
        "internal/platform/wifi_direct_test.cc",
        "internal/platform/wifi_hotspot_test.cc",
        "internal/platform/wifi_lan_test.cc",
        "internal/platform/wifi_test.cc",
        "internal/platform/connection_info_test.cc",
        "internal/platform/condition_variable_test.cc",
        "internal/platform/thread_check_nocompile_test.py",
        "internal/platform/bluetooth_classic_test.cc",
        "internal/platform/bluetooth_connection_info_test.cc",
        "internal/platform/mutex_test.cc",
        "internal/platform/atomic_reference_test.cc",
        "internal/platform/multi_thread_executor_test.cc",
        "internal/platform/ble_connection_info_test.cc",
        "internal/platform/ble_test.cc",
        "internal/platform/ble_v2_test.cc",
        "internal/platform/prng_test.cc",
        "internal/platform/implementation/apple/count_down_latch_test.cc",
        "internal/platform/implementation/apple/condition_variable_test.cc",
        "internal/platform/implementation/apple/mutex_test.cc",
        "internal/platform/implementation/apple/atomic_boolean_test.cc",
        "internal/platform/implementation/apple/atomic_uint32_test.cc",
        "internal/platform/implementation/shared/file_test.cc",
        "internal/platform/implementation/wifi_utils_test.cc",
        "internal/platform/atomic_boolean_test.cc",
        "internal/platform/exception_test.cc",
        "internal/platform/error_code_recorder_test.cc",
        "internal/platform/future_test.cc",
        "internal/platform/cancellation_flag_test.cc",
        "internal/platform/bluetooth_adapter_test.cc",
        "internal/platform/byte_utils_test.cc",
        "internal/platform/direct_executor_test.cc",
        "internal/platform/borrowable_test.cc",
        "internal/platform/implementation/windows/http_loader_test.cc",
        "internal/network/utils_test.cc",
        "internal/network/url_test.cc",
        "internal/network/http_response_test.cc",
        "internal/network/http_request_test.cc",
        "internal/network/http_client_impl_test.cc",
        "internal/network/http_status_code_test.cc",
        "internal/test/google3_only/fake_authentication_manager_test.cc",
        "internal/test/fake_clock_test.cc",
        "internal/test/fake_webrtc.cc",
        "internal/test/fake_timer_test.cc",
        "internal/test/fake_device_info_test.cc",
        "internal/test/fake_task_runner_test.cc",
        "internal/test/fake_data_set_test.cc",
        "internal/test/fake_http_client_test.cc",
        "internal/weave/base_socket_test.cc",
        "internal/weave/control_packet_write_request_test.cc",
        "internal/weave/message_write_request_test.cc",
        "internal/weave/packet_test.cc",
        "internal/weave/packet_sequence_number_generator_test.cc",
        "internal/weave/packetizer_test.cc",
        "internal/weave/sockets/client_socket_test.cc",
        "internal/weave/sockets/server_socket_test.cc",
        // simulation
        "connections/implementation/offline_simulation_user.cc",
        "connections/implementation/simulation_user.cc",
        // proto
        "connections/implementation/proto",
        "internal/proto",
        "proto",
        "internal/data/leveldb_data_set_test.proto",
        // webrtc
        "connections/implementation/webrtc_bwu_handler.cc",
        "connections/implementation/webrtc_endpoint_channel.cc",
        "connections/implementation/mediums/webrtc.cc",
        "connections/implementation/mediums/webrtc_peer_id.cc",
        "connections/implementation/mediums/webrtc",
        // This breaks the build, but seems to work fine without it?
        "internal/platform/medium_environment.cc",
      ],
      sources: [
        "compiled_proto",
        "connections",
        "internal",
        "proto",
      ],
      publicHeadersPath: "connections/swift/NearbyCoreAdapter/Sources/Public",
      cSettings: [
        .headerSearchPath("./"),
        .headerSearchPath("compiled_proto/"),
        .headerSearchPath("third_party/ukey2/ukey2/"),
        .headerSearchPath("third_party/ukey2/compiled_proto/"),
        .define("NO_WEBRTC"),
        .define("NEARBY_SWIFTPM"),
      ]
    ),
    .target(
      name: "NearbyConnections",
      dependencies: ["NearbyCoreAdapter"],
      path: ".",
      exclude: [
        "compiled_proto",
        "connections/c",
        "connections/dart",
        "connections/java",
        "connections/clients/ios",
        "connections/swift/NearbyCoreAdapter",
        "connections/swift/NearbyConnections/BUILD",
        "connections/swift/NearbyConnections/Tests",
        "connections/BUILD",
        "connections/discovery_options.cc",
        "connections/status.cc",
        "connections/listeners_test.cc",
        "connections/advertising_options.cc",
        "connections/payload.cc",
        "connections/strategy_test.cc",
        "connections/connection_options.cc",
        "connections/core_test.cc",
        "connections/status_test.cc",
        "connections/core.cc",
        "connections/strategy.cc",
        "connections/payload_test.cc",
        "connections/implementation",
        "docs",
        "embedded",
        "internal",
        "presence",
        "proto",
        "third_party",
        "CONTRIBUTING.md",
        "LICENSE",
        "README.md",
        "WORKSPACE",
      ],
      sources: [
        "connections/swift/NearbyConnections/Sources"
      ]
    ),
    .testTarget(
      name: "NearbyCoreAdapterTests",
      dependencies: ["NearbyCoreAdapter"],
      path: "connections/swift/NearbyCoreAdapter/Tests"
    ),
    .testTarget(
      name: "NearbyConnectionsTests",
      dependencies: ["NearbyConnections"],
      path: "connections/swift/NearbyConnections/Tests"
    ),
  ],
  cLanguageStandard: .c99,
  cxxLanguageStandard: .cxx17
)
