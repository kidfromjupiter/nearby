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

#include "sharing/linux/platform/linux_platform_components.h"

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "internal/base/file_path.h"
#include "internal/platform/mac_address.h"
#include "sharing/internal/api/private_certificate_data.h"
#include "sharing/linux/platform/platform_util.h"
#include "sharing/proto/rpc_resources.pb.h"

namespace nearby::sharing::linux::internal {
namespace {

using ::nearby::sharing::api::PreferenceManager;
using ::nearby::sharing::api::PublicCertificateDatabase;
using ::nearby::sharing::proto::PublicCertificate;

constexpr absl::string_view kAppFirstRunPref = "nearby_sharing.app.first_run";
constexpr absl::string_view kAppActivePref = "nearby_sharing.app.active";

class LinuxNetworkMonitor final : public nearby::api::NetworkMonitor {
 public:
  LinuxNetworkMonitor(std::function<void(bool)> lan_connected_callback,
                      std::function<void(bool)> internet_connected_callback)
      : nearby::api::NetworkMonitor(std::move(lan_connected_callback),
                                    std::move(internet_connected_callback)) {
    if (lan_connected_callback_) {
      lan_connected_callback_(IsLanConnected());
    }
    if (internet_connected_callback_) {
      internet_connected_callback_(IsInternetConnected());
    }
  }

  bool IsLanConnected() override { return HasNonLoopbackInterface(); }
  bool IsInternetConnected() override { return HasNonLoopbackInterface(); }
};

class LinuxSystemInfo final : public nearby::api::SystemInfo {
 public:
  std::string GetComputerManufacturer() override { return "Unknown"; }
  std::string GetComputerModel() override { return "Unknown"; }
  int64_t GetComputerPhysicalMemory() override {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
      return 0;
    }
    return static_cast<int64_t>(info.totalram) * info.mem_unit;
  }
  int GetComputerProcessorCount() override {
    long processors = sysconf(_SC_NPROCESSORS_CONF);
    return processors > 0 ? static_cast<int>(processors) : 1;
  }
  int GetComputerLogicProcessorCount() override {
    long processors = sysconf(_SC_NPROCESSORS_ONLN);
    return processors > 0 ? static_cast<int>(processors) : 1;
  }
  int GetProcessorMemoryInfo() override { return 0; }
  BatteryChargeStatus QueryBatteryInfo(int& seconds, int& percent,
                                       bool& battery_saver) override {
    seconds = 0;
    percent = 0;
    battery_saver = false;
    return BatteryChargeStatus::UNKNOWN;
  }
  std::string GetOsManufacturer() override { return "Linux"; }
  std::string GetOsName() override { return "Linux"; }
  std::string GetOsVersion() override {
    struct utsname info;
    return uname(&info) == 0 ? std::string(info.release) : std::string();
  }
  std::string GetOsArchitecture() override {
    struct utsname info;
    return uname(&info) == 0 ? std::string(info.machine) : std::string();
  }
  std::string GetOsLanguage() override {
    return GetLanguageCode().value_or("en");
  }
  std::string GetProcessorManufacturer() override { return "Unknown"; }
  std::string GetProcessorName() override { return "Unknown"; }
  std::list<DriverInfo> GetBluetoothDriverInfos() override { return {}; }
  std::list<DriverInfo> GetNetworkDriverInfos() override { return {}; }
  void GetBatteryUsageReport(const FilePath& save_path) override {
    static_cast<void>(save_path);
  }
};

class LinuxAppInfo final : public nearby::api::AppInfo {
 public:
  explicit LinuxAppInfo(PreferenceManager& preference_manager)
      : preference_manager_(preference_manager) {}

  std::optional<std::string> GetAppVersion() override {
    return std::string("linux");
  }
  std::optional<std::string> GetAppLanguage() override {
    return GetLanguageCode();
  }
  std::optional<std::string> GetUpdateTrack() override { return std::nullopt; }
  std::optional<std::string> GetAppInstallSource() override {
    return std::string("manual");
  }
  bool GetFirstRunDone() override {
    return preference_manager_.GetBoolean(kAppFirstRunPref, false);
  }
  bool SetFirstRunDone(bool value) override {
    preference_manager_.SetBoolean(kAppFirstRunPref, value);
    return true;
  }
  bool SetActiveFlag() override {
    preference_manager_.SetBoolean(kAppActivePref, true);
    return true;
  }

 private:
  PreferenceManager& preference_manager_;
};

class LinuxDeviceInfo final : public nearby::api::DeviceInfo {
 public:
  std::optional<std::string> GetOsDeviceName() const override {
    char hostname[256] = {};
    if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != '\0') {
      return std::string(hostname);
    }
    return std::string("Linux");
  }
  DeviceType GetDeviceType() const override { return DeviceType::kLaptop; }
  OsType GetOsType() const override { return OsType::kUnknown; }
  FilePath GetDownloadPath() const override {
    const char* xdg_download_dir = std::getenv("XDG_DOWNLOAD_DIR");
    if (xdg_download_dir != nullptr && *xdg_download_dir != '\0') {
      return FilePath(std::string(xdg_download_dir));
    }
    return BuildPathFromBase(GetHomeDirectory(), {"Downloads"});
  }
  FilePath GetLocalAppDataPath(FilePath sub_path) const override {
    std::string config_home = GetEnvOrDefault(
        "XDG_CONFIG_HOME",
        BuildPathFromBase(GetHomeDirectory(), {".config"}).ToString());
    FilePath path = BuildPathFromBase(config_home, {"Google Nearby"});
    if (!sub_path.IsEmpty()) {
      path.append(sub_path);
    }
    return path;
  }
  FilePath GetTemporaryPath() const override {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir != nullptr && *runtime_dir != '\0') {
      return BuildPathFromBase(runtime_dir, {"Google Nearby"});
    }
    return BuildPathFromBase("/tmp", {"Google Nearby"});
  }
  FilePath GetLogPath() const override {
    return GetQuickShareLogPath();
  }
  bool IsScreenLocked() const override { return false; }
  void RegisterScreenLockedListener(
      absl::string_view listener_name,
      std::function<void(ScreenStatus)> callback) override {
    screen_locked_listeners_[std::string(listener_name)] = std::move(callback);
  }
  void UnregisterScreenLockedListener(
      absl::string_view listener_name) override {
    screen_locked_listeners_.erase(std::string(listener_name));
  }
  bool PreventSleep() override { return true; }
  bool AllowSleep() override { return true; }

 private:
  mutable absl::flat_hash_map<std::string, std::function<void(ScreenStatus)>>
      screen_locked_listeners_;
};

class LinuxBluetoothAdapter final : public api::BluetoothAdapter {
 public:
  explicit LinuxBluetoothAdapter(
      std::shared_ptr<::nearby::linux::BluetoothAdapter> adapter)
      : adapter_(std::move(adapter)) {}

  bool IsPresent() const override { return GetAddress().IsSet(); }
  bool IsPowered() const override {
    return adapter_ != nullptr && adapter_->IsEnabled();
  }
  bool IsLowEnergySupported() const override { return adapter_ != nullptr; }
  bool IsScanOffloadSupported() const override { return false; }
  bool IsAdvertisementOffloadSupported() const override { return false; }
  bool IsExtendedAdvertisingSupported() const override { return false; }
  bool IsPeripheralRoleSupported() const override { return adapter_ != nullptr; }
  PermissionStatus GetOsPermissionStatus() const override {
    return adapter_ != nullptr ? PermissionStatus::kAllowed
                               : PermissionStatus::kSystemDenied;
  }
  void SetPowered(bool powered, std::function<void()> success_callback,
                  std::function<void()> error_callback) override {
    if (adapter_ == nullptr) {
      if (error_callback) {
        error_callback();
      }
      return;
    }
    bool success = adapter_->SetStatus(
        powered ? nearby::api::BluetoothAdapter::Status::kEnabled
                : nearby::api::BluetoothAdapter::Status::kDisabled);
    if (success) {
      if (success_callback) {
        success_callback();
      }
      return;
    }
    if (error_callback) {
      error_callback();
    }
  }
  std::optional<std::string> GetAdapterId() const override {
    if (adapter_ == nullptr) {
      return std::nullopt;
    }
    std::string name = adapter_->GetName();
    return name.empty() ? std::nullopt : std::make_optional(name);
  }
  MacAddress GetAddress() const override {
    return adapter_ != nullptr ? adapter_->GetMacAddress() : MacAddress();
  }
  void AddObserver(Observer* observer) override { observers_.insert(observer); }
  void RemoveObserver(Observer* observer) override {
    observers_.erase(observer);
  }
  bool HasObserver(Observer* observer) override {
    return observers_.contains(observer);
  }

 private:
  std::shared_ptr<::nearby::linux::BluetoothAdapter> adapter_;
  absl::flat_hash_set<Observer*> observers_;
};

class LinuxPublicCertificateDatabase final : public PublicCertificateDatabase {
 public:
  void Initialize(absl::AnyInvocable<void(InitStatus) &&> callback) override {
    if (callback) {
      std::move(callback)(InitStatus::kOk);
    }
  }
  void LoadEntries(
      absl::AnyInvocable<void(bool, std::unique_ptr<std::vector<
                                          PublicCertificate>>) &&>
          callback) override {
    auto certificates = std::make_unique<std::vector<PublicCertificate>>();
    {
      absl::MutexLock lock(mutex_);
      for (const auto& [id, certificate] : entries_) {
        static_cast<void>(id);
        certificates->push_back(certificate);
      }
    }
    if (callback) {
      std::move(callback)(true, std::move(certificates));
    }
  }
  void LoadCertificate(
      absl::string_view id,
      absl::AnyInvocable<void(bool, std::unique_ptr<PublicCertificate>) &&>
          callback) override {
    auto certificate = std::make_unique<PublicCertificate>();
    bool found = false;
    {
      absl::MutexLock lock(mutex_);
      auto it = entries_.find(std::string(id));
      if (it != entries_.end()) {
        *certificate = it->second;
        found = true;
      }
    }
    if (callback) {
      std::move(callback)(found, found ? std::move(certificate) : nullptr);
    }
  }
  void AddCertificates(absl::Span<const PublicCertificate> certificates,
                       absl::AnyInvocable<void(bool) &&> callback) override {
    {
      absl::MutexLock lock(mutex_);
      for (const PublicCertificate& certificate : certificates) {
        entries_[certificate.secret_id()] = certificate;
      }
    }
    if (callback) {
      std::move(callback)(true);
    }
  }
  void RemoveCertificatesById(
      std::vector<std::string> ids_to_remove,
      absl::AnyInvocable<void(bool) &&> callback) override {
    {
      absl::MutexLock lock(mutex_);
      for (const std::string& id : ids_to_remove) {
        entries_.erase(id);
      }
    }
    if (callback) {
      std::move(callback)(true);
    }
  }
  void Destroy(absl::AnyInvocable<void(bool) &&> callback) override {
    {
      absl::MutexLock lock(mutex_);
      entries_.clear();
    }
    if (callback) {
      std::move(callback)(true);
    }
  }

 private:
  absl::Mutex mutex_;
  std::map<std::string, PublicCertificate> entries_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace

std::unique_ptr<nearby::api::NetworkMonitor> CreateLinuxNetworkMonitor(
    std::function<void(bool)> lan_connected_callback,
    std::function<void(bool)> internet_connected_callback) {
  return std::make_unique<LinuxNetworkMonitor>(
      std::move(lan_connected_callback), std::move(internet_connected_callback));
}

std::unique_ptr<nearby::api::SystemInfo> CreateLinuxSystemInfo() {
  return std::make_unique<LinuxSystemInfo>();
}

std::unique_ptr<nearby::api::AppInfo> CreateLinuxAppInfo(
    api::PreferenceManager& preference_manager) {
  return std::make_unique<LinuxAppInfo>(preference_manager);
}

std::unique_ptr<nearby::api::DeviceInfo> CreateLinuxDeviceInfo() {
  return std::make_unique<LinuxDeviceInfo>();
}

std::unique_ptr<api::BluetoothAdapter> CreateLinuxBluetoothAdapter(
    std::shared_ptr<::nearby::linux::BluetoothAdapter> adapter) {
  return std::make_unique<LinuxBluetoothAdapter>(std::move(adapter));
}

std::unique_ptr<api::PublicCertificateDatabase>
CreateLinuxPublicCertificateDatabase() {
  return std::make_unique<LinuxPublicCertificateDatabase>();
}

}  // namespace nearby::sharing::linux::internal
