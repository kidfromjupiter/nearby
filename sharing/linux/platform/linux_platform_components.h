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

#ifndef SHARING_LINUX_PLATFORM_LINUX_PLATFORM_COMPONENTS_H_
#define SHARING_LINUX_PLATFORM_LINUX_PLATFORM_COMPONENTS_H_

#include <functional>
#include <memory>

#include "internal/platform/implementation/device_info.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "sharing/internal/api/app_info.h"
#include "sharing/internal/api/bluetooth_adapter.h"
#include "sharing/internal/api/network_monitor.h"
#include "sharing/internal/api/preference_manager.h"
#include "sharing/internal/api/public_certificate_database.h"
#include "sharing/internal/api/system_info.h"

namespace nearby::sharing::linux::internal {

std::unique_ptr<nearby::api::NetworkMonitor> CreateLinuxNetworkMonitor(
    std::function<void(bool)> lan_connected_callback,
    std::function<void(bool)> internet_connected_callback);
std::unique_ptr<nearby::api::SystemInfo> CreateLinuxSystemInfo();
std::unique_ptr<nearby::api::AppInfo> CreateLinuxAppInfo(
    api::PreferenceManager& preference_manager);
std::unique_ptr<nearby::api::DeviceInfo> CreateLinuxDeviceInfo();
std::unique_ptr<api::BluetoothAdapter> CreateLinuxBluetoothAdapter(
    std::shared_ptr<::nearby::linux::BluetoothAdapter> adapter);
std::unique_ptr<api::PublicCertificateDatabase>
CreateLinuxPublicCertificateDatabase();

}  // namespace nearby::sharing::linux::internal

#endif  // SHARING_LINUX_PLATFORM_LINUX_PLATFORM_COMPONENTS_H_
