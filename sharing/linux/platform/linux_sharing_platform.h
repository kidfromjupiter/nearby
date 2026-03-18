// Copyright 2026

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "internal/base/file_path.h"
#include "internal/platform/device_info.h"
#include "internal/platform/implementation/account_manager.h"
#include "internal/platform/task_runner.h"
#include "sharing/internal/api/sharing_platform.h"

namespace nearby::sharing::linux {

class LinuxSharingPlatform final : public nearby::sharing::api::SharingPlatform {
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

  nearby::sharing::api::BluetoothAdapter& GetBluetoothAdapter() override;
  nearby::api::FastInitBleBeacon& GetFastInitBleBeacon() override;
  nearby::api::FastInitiationManager& GetFastInitiationManager() override;
  std::unique_ptr<nearby::api::SystemInfo> CreateSystemInfo() override;
  std::unique_ptr<nearby::api::AppInfo> CreateAppInfo() override;
  nearby::sharing::api::PreferenceManager& GetPreferenceManager() override;
  AccountManager& GetAccountManager() override;
  TaskRunner& GetDefaultTaskRunner() override;
  nearby::DeviceInfo& GetDeviceInfo() override;
  std::unique_ptr<nearby::sharing::api::PublicCertificateDatabase>
  CreatePublicCertificateDatabase(const FilePath& database_path) override;
  bool UpdateFileOriginMetadata(std::vector<FilePath>& file_paths) override;

 private:
  void Initialize(std::string device_name_override);

  std::unique_ptr<nearby::sharing::api::PreferenceManager> preference_manager_;
  std::unique_ptr<AccountManager> account_manager_;
  std::unique_ptr<nearby::sharing::api::BluetoothAdapter> bluetooth_adapter_;
  std::unique_ptr<nearby::api::FastInitBleBeacon> fast_init_ble_beacon_;
  std::unique_ptr<nearby::api::FastInitiationManager> fast_initiation_manager_;
  std::unique_ptr<TaskRunner> default_task_runner_;
  std::unique_ptr<nearby::DeviceInfo> device_info_;
  absl::string_view (*product_id_getter_)() = nullptr;
};

}  // namespace nearby::sharing::linux

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_PLATFORM_LINUX_SHARING_PLATFORM_H_
