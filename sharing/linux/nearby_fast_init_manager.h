
// Copyright 2022 Google LLC
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

#ifndef LINUX_NEARBY_SHARING_INTERNAL_API_FAST_INITIATION_MANAGER_H_
#define LINUX_NEARBY_SHARING_INTERNAL_API_FAST_INITIATION_MANAGER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/implementation/ble.h"
#include "sharing/internal/api/fast_init_ble_beacon.h"
#include "sharing/internal/api/fast_initiation_manager.h"

#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluetooth_devices.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/bluez_advertisement_monitor.h"
#include "internal/platform/implementation/linux/bluez_advertisement_monitor_manager.h"
#include "internal/platform/implementation/linux/bluez_le_advertisement.h"

constexpr char kFastInitServiceUuid[] = "0000fe2c-0000-1000-8000-00805f9b34fb";

namespace nearby {
namespace sharing {
namespace linux {

class LinuxFastInitiationManager final
    : public nearby::api::FastInitiationManager {
 public:
  explicit LinuxFastInitiationManager(
      nearby::api::FastInitBleBeacon& beacon,
      std::shared_ptr<::nearby::linux::BluetoothAdapter> bluetooth_adapter)
      : beacon_(beacon), adapter_(std::move(bluetooth_adapter)) {
    if (adapter_ != nullptr) {
      adv_manager_ =
          std::make_unique<nearby::linux::bluez::LEAdvertisementManager>(
              *adapter_->GetConnection(), *adapter_);
    }
  }
  ~LinuxFastInitiationManager() override;

  void StartAdvertising(
      nearby::api::FastInitBleBeacon::FastInitType type,
      std::function<void()> callback,
      std::function<void(nearby::api::FastInitiationManager::Error)>
          error_callback) override;

  void StopAdvertising(std::function<void()> callback) override;

  void StartScanning(
      std::function<void()> devices_discovered_callback,
      std::function<void()> devices_not_discovered_callback,
      std::function<void(nearby::api::FastInitiationManager::Error)>
          error_callback) override;

  void StopScanning(std::function<void()> callback) override;

  bool IsAdvertising() override {
    absl::MutexLock lock(mutex_);
    return advertisement_ != nullptr;
  }
  bool IsScanning() override;

 private:
  friend class LinuxFastInitiationManagerTestPeer;

  void OnAdvertisementFound(
      uint64_t scan_generation,
      ::nearby::api::ble::BlePeripheral::UniqueId peripheral_id,
      ::nearby::api::ble::BleAdvertisementData advertisement_data);
  void OnAdvertisementLost(
      uint64_t scan_generation,
      ::nearby::api::ble::BlePeripheral::UniqueId peripheral_id);
  bool IsFastInitAdvertisement(
      const ::nearby::api::ble::BleAdvertisementData& advertisement_data) const;
  std::optional<nearby::api::FastInitiationManager::Error>
  StartScanningInternal(std::function<void()> devices_discovered_callback,
                        std::function<void()> devices_not_discovered_callback)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(scan_operation_mutex_);
  void StopScanningInternal()
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(scan_operation_mutex_);

  nearby::api::FastInitBleBeacon& beacon_;
  std::shared_ptr<::nearby::linux::BluetoothAdapter> adapter_;
  std::unique_ptr<::nearby::linux::bluez::LEAdvertisementManager> adv_manager_;
  absl::Mutex scan_operation_mutex_;
  absl::Mutex mutex_;
  std::unique_ptr<::nearby::linux::bluez::LEAdvertisement> advertisement_
      ABSL_GUARDED_BY(mutex_);

  bool is_scanning_ ABSL_GUARDED_BY(mutex_) = false;
  bool owns_bluez_discovery_ ABSL_GUARDED_BY(mutex_) = false;
  // Incremented for every start/stop so delayed BlueZ callbacks from an old
  // scan cannot update the device set or notify callbacks for a newer scan.
  uint64_t scan_generation_ ABSL_GUARDED_BY(mutex_) = 0;
  std::function<void()> devices_discovered_callback_ ABSL_GUARDED_BY(mutex_);
  std::function<void()> devices_not_discovered_callback_
      ABSL_GUARDED_BY(mutex_);
  absl::flat_hash_set<::nearby::api::ble::BlePeripheral::UniqueId>
      discovered_peripherals_ ABSL_GUARDED_BY(mutex_);
  std::shared_ptr<::nearby::linux::BluetoothDevices> scan_devices_
      ABSL_GUARDED_BY(scan_operation_mutex_);
  std::unique_ptr<::nearby::linux::RootObjectManager> scan_root_object_manager_
      ABSL_GUARDED_BY(scan_operation_mutex_);
  std::unique_ptr<::nearby::linux::bluez::AdvertisementMonitorManager>
      scan_monitor_manager_ ABSL_GUARDED_BY(scan_operation_mutex_);
  std::unique_ptr<::nearby::linux::bluez::AdvertisementMonitor> scan_monitor_
      ABSL_GUARDED_BY(scan_operation_mutex_);
};

}  // namespace linux
}  // namespace sharing
}  // namespace nearby

#endif  // LINUX_NEARBY_SHARING_INTERNAL_API_FAST_INITIATION_MANAGER_H_
