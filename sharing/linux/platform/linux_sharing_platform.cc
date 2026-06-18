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

#include "sharing/linux/platform/linux_sharing_platform.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "internal/base/file_path.h"
#include "sharing/internal/public/pref_names.h"
#include "sharing/linux/nearby_fast_init_ble_beacon.h"
#include "sharing/linux/nearby_fast_init_manager.h"
#include "sharing/linux/platform/linux_account_manager.h"
#include "sharing/linux/platform/linux_platform_components.h"
#include "sharing/linux/platform/linux_preference_manager.h"
#include "sharing/linux/platform/platform_util.h"

namespace nearby::sharing::linux {

LinuxSharingPlatform::LinuxSharingPlatform() { Initialize({}); }

LinuxSharingPlatform::LinuxSharingPlatform(std::string device_name_override) {
  Initialize(std::move(device_name_override));
}

LinuxSharingPlatform::~LinuxSharingPlatform() = default;

void LinuxSharingPlatform::Initialize(std::string device_name_override) {
  fast_init_adapter_ = internal::CreateFastInitBluetoothAdapter();
  preference_manager_ = internal::CreateLinuxPreferenceManager();
  account_manager_ = internal::CreateLinuxAccountManager();
  bluetooth_adapter_ =
      internal::CreateLinuxBluetoothAdapter(fast_init_adapter_);
  fast_init_ble_beacon_ = std::make_unique<LinuxFastInitBleBeacon>();
  fast_initiation_manager_ =
      std::make_unique<LinuxFastInitiationManager>(*fast_init_ble_beacon_,
                                                   fast_init_adapter_);
  device_info_ = internal::CreateLinuxDeviceInfo();

  if (!device_name_override.empty()) {
    preference_manager_->SetString(PrefNames::kDeviceName,
                                   device_name_override);
  }
}

void LinuxSharingPlatform::InitProductIdGetter(
    absl::string_view (*product_id_getter)()) {
  product_id_getter_ = product_id_getter;
}

std::unique_ptr<nearby::api::NetworkMonitor>
LinuxSharingPlatform::CreateNetworkMonitor(
    std::function<void(bool)> lan_connected_callback,
    std::function<void(bool)> internet_connected_callback) {
  return internal::CreateLinuxNetworkMonitor(
      std::move(lan_connected_callback), std::move(internet_connected_callback));
}

api::BluetoothAdapter& LinuxSharingPlatform::GetBluetoothAdapter() {
  return *bluetooth_adapter_;
}

nearby::api::FastInitBleBeacon& LinuxSharingPlatform::GetFastInitBleBeacon() {
  return *fast_init_ble_beacon_;
}

nearby::api::FastInitiationManager&
LinuxSharingPlatform::GetFastInitiationManager() {
  return *fast_initiation_manager_;
}

std::unique_ptr<nearby::api::SystemInfo>
LinuxSharingPlatform::CreateSystemInfo() {
  return internal::CreateLinuxSystemInfo();
}

std::unique_ptr<nearby::api::AppInfo> LinuxSharingPlatform::CreateAppInfo() {
  return internal::CreateLinuxAppInfo(*preference_manager_);
}

api::PreferenceManager& LinuxSharingPlatform::GetPreferenceManager() {
  return *preference_manager_;
}

AccountManager& LinuxSharingPlatform::GetAccountManager() {
  return *account_manager_;
}

nearby::api::DeviceInfo& LinuxSharingPlatform::GetDeviceInfo() {
  return *device_info_;
}

std::unique_ptr<api::PublicCertificateDatabase>
LinuxSharingPlatform::CreatePublicCertificateDatabase(
    const FilePath& database_path) {
  static_cast<void>(database_path);
  return internal::CreateLinuxPublicCertificateDatabase();
}

bool LinuxSharingPlatform::UpdateFileOriginMetadata(
    std::vector<FilePath>& file_paths) {
  static_cast<void>(file_paths);
  return true;
}

}  // namespace nearby::sharing::linux
