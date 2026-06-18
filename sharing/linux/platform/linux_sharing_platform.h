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

#ifndef SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_
#define SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_

#include <memory>
#include <string>

#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "location/nearby/sharing/lib/account/account_manager.h"
#include "sharing/internal/api/sharing_platform.h"

namespace nearby::sharing::linux {

class LinuxSharingPlatform final : public api::SharingPlatform {
 public:
  LinuxSharingPlatform();
  explicit LinuxSharingPlatform(std::string device_name_override);
  ~LinuxSharingPlatform() override;

  LinuxSharingPlatform(const LinuxSharingPlatform&) = delete;
  LinuxSharingPlatform& operator=(const LinuxSharingPlatform&) = delete;

  void InitProductIdGetter(
      absl::string_view (*product_id_getter)()) override;

  std::unique_ptr<nearby::api::NetworkMonitor> CreateNetworkMonitor(
      std::function<void(bool)> lan_connected_callback,
      std::function<void(bool)> internet_connected_callback) override;

  api::BluetoothAdapter& GetBluetoothAdapter() override;
  nearby::api::FastInitBleBeacon& GetFastInitBleBeacon() override;
  nearby::api::FastInitiationManager& GetFastInitiationManager() override;
  std::unique_ptr<nearby::api::SystemInfo> CreateSystemInfo() override;
  std::unique_ptr<nearby::api::AppInfo> CreateAppInfo() override;
  api::PreferenceManager& GetPreferenceManager() override;
  AccountManager& GetAccountManager() override;
  nearby::api::DeviceInfo& GetDeviceInfo() override;
  std::unique_ptr<api::PublicCertificateDatabase>
  CreatePublicCertificateDatabase(const FilePath& database_path) override;
  bool UpdateFileOriginMetadata(std::vector<FilePath>& file_paths) override;

 private:
  void Initialize(std::string device_name_override);

  std::shared_ptr<::nearby::linux::BluetoothAdapter> fast_init_adapter_;
  std::unique_ptr<api::PreferenceManager> preference_manager_;
  std::unique_ptr<AccountManager> account_manager_;
  std::unique_ptr<api::BluetoothAdapter> bluetooth_adapter_;
  std::unique_ptr<nearby::api::FastInitBleBeacon> fast_init_ble_beacon_;
  std::unique_ptr<nearby::api::FastInitiationManager>
      fast_initiation_manager_;
  std::unique_ptr<nearby::api::DeviceInfo> device_info_;
  absl::string_view (*product_id_getter_)() = nullptr;
};

}  // namespace nearby::sharing::linux

#endif  // SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_

