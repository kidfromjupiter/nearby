// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <signal.h>

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "absl/synchronization/notification.h"
#include "internal/platform/bluetooth_adapter.h"
#include "internal/platform/system_clock.h"
#include "internal/proto/credential.pb.h"
#include "presence/power_mode.h"
#include "presence/presence_device.h"
#include "presence/presence_service_impl.h"
#include "presence/scan_request.h"

#if defined(__linux__)
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluetooth_devices.h"
#endif

namespace {

absl::Notification g_shutdown;

void QuitHandler(int, siginfo_t*, void*) {
  if (!g_shutdown.HasBeenNotified()) g_shutdown.Notify();
}

std::string RangeTypeToString(
    const std::string& range) {
  return range;
}

std::string EstimateZone(double distance_meters) {
  constexpr double kTapDistanceThresholdMeters = 0.08;
  constexpr double kReachDistanceThresholdMeters = 0.56;
  constexpr double kShortRangeDistanceThresholdMeters = 1.26;
  constexpr double kLongRangeDistanceThresholdMeters = 3.06;

  if (distance_meters <= kTapDistanceThresholdMeters) return "tap";
  if (distance_meters <= kReachDistanceThresholdMeters) return "reach";
  if (distance_meters <= kShortRangeDistanceThresholdMeters) return "short";
  if (distance_meters <= kLongRangeDistanceThresholdMeters) return "long";
  return "far";
}

double EstimateDistanceMeters(int adjusted_rssi) {
  // Mirrors presence/fpp/fpp/src/fspl_converter.rs.
  constexpr int kTxPowerAtZeroMetersDb = -20;
  constexpr int kFsplAtOneMeterDb = 40;
  const int fspl = kTxPowerAtZeroMetersDb - adjusted_rssi;
  return std::pow(10.0, static_cast<double>(fspl - kFsplAtOneMeterDb) / 20.0);
}

std::string FormatProximity(int rssi, std::optional<int8_t> tx_power) {
  const int adjusted_rssi = rssi + tx_power.value_or(0);
  const double distance_meters = EstimateDistanceMeters(adjusted_rssi);
  const std::string zone = EstimateZone(distance_meters);
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << "distance_m="
         << distance_meters << " zone=" << RangeTypeToString(zone)
         << " rssi=" << rssi;
  if (tx_power.has_value()) {
    stream << " tx_power=" << static_cast<int>(*tx_power);
  }
  return stream.str();
}

std::string FormatProximityUnavailable() { return "proximity=unavailable"; }

std::string IdentityTypeToString(nearby::internal::IdentityType identity_type) {
  switch (identity_type) {
    case nearby::internal::IDENTITY_TYPE_PUBLIC:
      return "public";
    case nearby::internal::IDENTITY_TYPE_PRIVATE_GROUP:
      return "private_group";
    case nearby::internal::IDENTITY_TYPE_CONTACTS_GROUP:
      return "contacts_group";
    default:
      return "unspecified";
  }
}

std::optional<uint64_t> ParseHexId(const std::string& hex_id) {
  if (hex_id.empty()) return std::nullopt;
  try {
    size_t parsed_size = 0;
    uint64_t id = std::stoull(hex_id, &parsed_size, 16);
    if (parsed_size != hex_id.size()) return std::nullopt;
    return id;
  } catch (...) {
    return std::nullopt;
  }
}

class SignalProvider {
 public:
  SignalProvider() {
#if defined(__linux__)
    nearby::BluetoothAdapter adapter;
    auto* linux_adapter =
        dynamic_cast<nearby::linux::BluetoothAdapter*>(&adapter.GetImpl());
    if (linux_adapter == nullptr) {
      return;
    }
    shared_devices_ = nearby::linux::GetSharedBluetoothDevices(
        linux_adapter->GetConnection(), linux_adapter->GetObjectPath());
#endif
  }

  std::optional<int> GetRssi(uint64_t device_id) const {
#if defined(__linux__)
    if (!shared_devices_) return std::nullopt;
    auto device = shared_devices_->devices->get_device_by_unique_id(device_id);
    if (!device) return std::nullopt;
    auto rssi = device->GetRssi();
    if (!rssi.has_value()) return std::nullopt;
    return static_cast<int>(*rssi);
#else
    (void)device_id;
    return std::nullopt;
#endif
  }

  std::optional<int8_t> GetTxPower(uint64_t device_id) const {
#if defined(__linux__)
    if (!shared_devices_) return std::nullopt;
    auto device = shared_devices_->devices->get_device_by_unique_id(device_id);
    if (!device) return std::nullopt;
    auto tx_power = device->GetTxPower();
    if (!tx_power.has_value()) return std::nullopt;
    if (*tx_power < std::numeric_limits<int8_t>::min() ||
        *tx_power > std::numeric_limits<int8_t>::max()) {
      return std::nullopt;
    }
    return static_cast<int8_t>(*tx_power);
#else
    (void)device_id;
    return std::nullopt;
#endif
  }

 private:
#if defined(__linux__)
  std::shared_ptr<nearby::linux::SharedBluetoothDevices> shared_devices_;
#endif
};

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " [--power low|balanced|low-latency] [--screen-on-only]"
            << " [--identities all|public|private|contacts|csv]"
            << " [--manager-app-id <id>] [--account-name <name>]\n";
}

nearby::presence::PowerMode ParsePowerMode(const std::string& arg) {
  if (arg == "low") return nearby::presence::PowerMode::kLowPower;
  if (arg == "low-latency") return nearby::presence::PowerMode::kLowLatency;
  return nearby::presence::PowerMode::kBalanced;
}

bool AddIdentityTypeFromToken(
    const std::string& token,
    std::vector<nearby::internal::IdentityType>& identity_types) {
  using IdentityType = nearby::internal::IdentityType;
  IdentityType identity = IdentityType::IDENTITY_TYPE_UNSPECIFIED;
  if (token == "public") {
    identity = IdentityType::IDENTITY_TYPE_PUBLIC;
  } else if (token == "private") {
    identity = IdentityType::IDENTITY_TYPE_PRIVATE_GROUP;
  } else if (token == "contacts") {
    identity = IdentityType::IDENTITY_TYPE_CONTACTS_GROUP;
  } else {
    return false;
  }

  for (auto existing : identity_types) {
    if (existing == identity) return true;
  }
  identity_types.push_back(identity);
  return true;
}

bool ParseIdentityTypes(
    const std::string& arg,
    std::vector<nearby::internal::IdentityType>& identity_types) {
  identity_types.clear();
  if (arg == "all") {
    return true;
  }
  size_t start = 0;
  while (start < arg.size()) {
    size_t comma = arg.find(',', start);
    std::string token = arg.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!AddIdentityTypeFromToken(token, identity_types)) return false;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return !identity_types.empty();
}

bool ContainsIdentityType(const std::vector<nearby::internal::IdentityType>& ids,
                          nearby::internal::IdentityType identity_type) {
  for (auto id : ids) {
    if (id == identity_type) return true;
  }
  return false;
}

std::string IdentitySelectionToString(
    const std::vector<nearby::internal::IdentityType>& identity_types) {
  if (identity_types.empty()) {
    return "all";
  }
  std::string result;
  for (size_t i = 0; i < identity_types.size(); ++i) {
    if (i > 0) result += ",";
    result += IdentityTypeToString(identity_types[i]);
  }
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  (void)nearby::SystemClock::ElapsedRealtime();

  nearby::presence::PowerMode power_mode = nearby::presence::PowerMode::kBalanced;
  bool screen_on_only = false;
  std::string identities_arg = "all";
  std::string manager_app_id;
  std::string account_name;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-h") || (arg == "--help")) {
      PrintUsage(argv[0]);
      return 0;
    }
    if (arg == "--power") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --power\n";
        return 2;
      }
      power_mode = ParsePowerMode(argv[++i]);
      continue;
    }
    if (arg == "--screen-on-only") {
      screen_on_only = true;
      continue;
    }
    if (arg == "--identities") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --identities\n";
        return 2;
      }
      identities_arg = argv[++i];
      continue;
    }
    if (arg == "--manager-app-id") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --manager-app-id\n";
        return 2;
      }
      manager_app_id = argv[++i];
      continue;
    }
    if (arg == "--account-name") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --account-name\n";
        return 2;
      }
      account_name = argv[++i];
      continue;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    PrintUsage(argv[0]);
    return 2;
  }

  std::vector<nearby::internal::IdentityType> identity_types;
  if (!ParseIdentityTypes(identities_arg, identity_types)) {
    std::cerr << "Invalid --identities value: " << identities_arg << "\n";
    PrintUsage(argv[0]);
    return 2;
  }

  struct sigaction action {};
  action.sa_sigaction = QuitHandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);

  SignalProvider signal_provider;
  nearby::presence::PresenceServiceImpl service;
  auto client = service.CreatePresenceClient();

  nearby::presence::ScanRequest request;
  request.use_ble = true;
  request.scan_type = nearby::presence::ScanType::kPresenceScan;
  request.power_mode = power_mode;
  request.scan_only_when_screen_on = screen_on_only;
  request.manager_app_id = manager_app_id;
  request.account_name = account_name;
  request.identity_types = identity_types;

  const bool scans_non_public =
      request.identity_types.empty() ||
      ContainsIdentityType(request.identity_types,
                           nearby::internal::IdentityType::
                               IDENTITY_TYPE_PRIVATE_GROUP) ||
      ContainsIdentityType(request.identity_types,
                           nearby::internal::IdentityType::
                               IDENTITY_TYPE_CONTACTS_GROUP);
  if (scans_non_public && request.manager_app_id.empty()) {
    std::cerr << "Warning: scanning private/contact identities without "
                 "--manager-app-id usually results in empty remote credentials.\n";
  }

  absl::Status start_status;
  absl::Notification started;
  auto scan_session = client->StartScan(
      request,
      nearby::presence::ScanCallback{
          .start_scan_cb =
              [&start_status, &started](absl::Status status) {
                start_status = status;
                started.Notify();
              },
          .on_discovered_cb =
              [&signal_provider](nearby::presence::PresenceDevice device) {
                std::string remote_hex =
                    device.GetDeviceIdentityMetadata().bluetooth_mac_address();
                std::optional<uint64_t> device_id = ParseHexId(remote_hex);
                if (!device_id.has_value()) {
                  std::cout << "[DISCOVERED] endpoint=" << device.GetEndpointId()
                            << " identity="
                            << IdentityTypeToString(device.GetIdentityType())
                            << " remote_hex=" << remote_hex
                            << " " << FormatProximityUnavailable() << "\n";
                  return;
                }
                std::optional<int> rssi = signal_provider.GetRssi(*device_id);
                std::optional<int8_t> tx_power =
                    signal_provider.GetTxPower(*device_id);
                std::string proximity_desc = FormatProximityUnavailable();
                if (rssi.has_value()) {
                  proximity_desc = FormatProximity(*rssi, tx_power);
                }
                std::cout << "[DISCOVERED] endpoint=" << device.GetEndpointId()
                          << " identity="
                          << IdentityTypeToString(device.GetIdentityType())
                          << " remote_hex=" << remote_hex << " "
                          << proximity_desc << "\n";
              },
          .on_updated_cb =
              [&signal_provider](nearby::presence::PresenceDevice device) {
                std::string remote_hex =
                    device.GetDeviceIdentityMetadata().bluetooth_mac_address();
                std::optional<uint64_t> device_id = ParseHexId(remote_hex);
                if (!device_id.has_value()) {
                  std::cout << "[UPDATED] endpoint=" << device.GetEndpointId()
                            << " identity="
                            << IdentityTypeToString(device.GetIdentityType())
                            << " remote_hex=" << remote_hex
                            << " " << FormatProximityUnavailable() << "\n";
                  return;
                }
                std::optional<int> rssi = signal_provider.GetRssi(*device_id);
                std::optional<int8_t> tx_power =
                    signal_provider.GetTxPower(*device_id);
                std::string proximity_desc = FormatProximityUnavailable();
                if (rssi.has_value()) {
                  proximity_desc = FormatProximity(*rssi, tx_power);
                }
                std::cout << "[UPDATED] endpoint=" << device.GetEndpointId()
                          << " identity="
                          << IdentityTypeToString(device.GetIdentityType())
                          << " remote_hex=" << remote_hex << " "
                          << proximity_desc << "\n";
              },
          .on_lost_cb =
              [](nearby::presence::PresenceDevice device) {
                std::cout << "[LOST] endpoint=" << device.GetEndpointId()
                          << " remote_hex="
                          << device.GetDeviceIdentityMetadata()
                                 .bluetooth_mac_address()
                          << "\n";
              }});

  if (!scan_session.ok()) {
    std::cerr << "Failed to initiate scan: " << scan_session.status() << "\n";
    return 1;
  }

  started.WaitForNotification();
  if (!start_status.ok()) {
    std::cerr << "Scan start failed: " << start_status << "\n";
    client->StopScan(*scan_session);
    return 1;
  }

  std::cout << "Presence scan started.\n"
            << "  session_id: " << *scan_session << "\n"
            << "  identities: "
            << IdentitySelectionToString(request.identity_types) << "\n"
            << "  manager_app_id: "
            << (request.manager_app_id.empty() ? "<empty>"
                                               : request.manager_app_id)
            << "\n"
            << "  account_name: "
            << (request.account_name.empty() ? "<empty>" : request.account_name)
            << "\n"
            << "  mode: printing discovered/updated/lost events with proximity "
               "estimates when RSSI is available\n"
            << "Press Ctrl+C to stop.\n";

  g_shutdown.WaitForNotification();
  client->StopScan(*scan_session);
  std::cout << "Stopped Presence scan.\n";
  return 0;
}
