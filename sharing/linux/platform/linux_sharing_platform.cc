// Copyright 2026

#include "sharing/linux/platform/linux_sharing_platform.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <sdbus-c++/Error.h>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "internal/base/file_path.h"
#include "internal/base/files.h"
#include "internal/platform/bluetooth_adapter.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/implementation/account_info.h"
#include "internal/platform/implementation/auth_status.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/bluez_le_advertisement.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/platform.h"
#include "internal/platform/mac_address.h"
#include "internal/platform/task_runner_impl.h"
#include "internal/platform/uuid.h"
#include "nlohmann/json.hpp"
#include "sharing/internal/api/app_info.h"
#include "sharing/internal/api/bluetooth_adapter.h"
#include "sharing/internal/api/fast_init_ble_beacon.h"
#include "sharing/internal/api/fast_initiation_manager.h"
#include "sharing/internal/api/network_monitor.h"
#include "sharing/internal/api/preference_manager.h"
#include "sharing/internal/api/private_certificate_data.h"
#include "sharing/internal/api/public_certificate_database.h"
#include "sharing/internal/api/system_info.h"
#include "sharing/internal/public/pref_names.h"
#include "sharing/proto/rpc_resources.pb.h"

namespace nearby::sharing {
namespace {

using Json = nlohmann::json;
using ::nearby::sharing::api::PreferenceManager;
using ::nearby::sharing::api::PrivateCertificateData;
using ::nearby::sharing::api::PublicCertificateDatabase;
using ::nearby::sharing::PrefNames;
using ::nearby::sharing::proto::PublicCertificate;
using ::nearby::sharing::sync::SyncBindingPrefs;
using ::nearby::sharing::sync::SyncConfigPrefs;

constexpr absl::string_view kPreferencesFile = "nearby_sharing.json";
constexpr absl::string_view kFileSyncBindingName = "FileSync";
constexpr absl::string_view kAppFirstRunPref = "nearby_sharing.app.first_run";
constexpr absl::string_view kAppActivePref = "nearby_sharing.app.active";
constexpr char kFastInitServiceUuid[] = "0000fe2c-0000-1000-8000-00805f9b34fb";

std::optional<Uuid> GetFastInitUuid() {
  return Uuid::FromString(kFastInitServiceUuid);
}

std::string GetFastInitSeed(
    nearby::api::FastInitBleBeacon::FastInitType type,
    const ::nearby::linux::BluetoothAdapter* adapter) {
  std::string seed = absl::StrCat("linux-fast-init:", static_cast<int>(type));
  if (adapter != nullptr) {
    seed = absl::StrCat(seed, ":", adapter->GetMacAddress().ToString());
  }
  return seed;
}

std::array<uint8_t, nearby::api::FastInitBleBeacon::kSecretIdHashSize>
BuildFastInitSecretIdHash(absl::string_view seed) {
  std::array<uint8_t, nearby::api::FastInitBleBeacon::kSecretIdHashSize> hash{};
  const std::uint64_t value = std::hash<std::string>{}(std::string(seed));
  for (size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xff);
  }
  return hash;
}

std::array<uint8_t, nearby::api::FastInitBleBeacon::kSaltSize> BuildFastInitSalt(
    absl::string_view seed) {
  return {static_cast<uint8_t>(std::hash<std::string>{}(std::string(seed)) &
                               0xff)};
}

std::array<uint8_t, nearby::api::FastInitBleBeacon::kAdvertiseDataTotalSize>
BuildFastInitAdvertisingData(nearby::api::FastInitBleBeacon& beacon) {
  std::array<uint8_t, nearby::api::FastInitBleBeacon::kAdvertiseDataTotalSize>
      data{};
  size_t offset = 0;

  data[offset++] = nearby::api::FastInitBleBeacon::kFastInitServiceUuid[0];
  data[offset++] = nearby::api::FastInitBleBeacon::kFastInitServiceUuid[1];

  for (uint8_t byte : nearby::api::FastInitBleBeacon::kFastInitModelId) {
    data[offset++] = byte;
  }

  const uint8_t metadata =
      (static_cast<uint8_t>(beacon.GetVersion()) << 5) |
      (static_cast<uint8_t>(beacon.GetType()) << 2) |
      (beacon.GetUwbSupported() ? 0x02 : 0x00) |
      (beacon.GetSenderCertSupported() ? 0x01 : 0x00);
  data[offset++] = metadata;
  data[offset++] = static_cast<uint8_t>(beacon.GetAdjustedTxPower());

  for (uint8_t byte : beacon.GetUwbMetadata()) {
    data[offset++] = byte;
  }
  for (uint8_t byte : beacon.GetUwbAddress()) {
    data[offset++] = byte;
  }
  for (uint8_t byte : beacon.GetSalt()) {
    data[offset++] = byte;
  }
  for (uint8_t byte : beacon.GetSecretIdHash()) {
    data[offset++] = byte;
  }

  data[offset++] = (beacon.GetRequireBtAdvertising() ? 0x80 : 0x00) |
                   (beacon.GetSelfOnlyAdvertising() ? 0x40 : 0x00);
  return data;
}

std::unique_ptr<::nearby::linux::BluetoothAdapter> CreateFastInitBluetoothAdapter() {
  auto system_bus = ::nearby::linux::getSystemBusConnection();
  auto manager = ::nearby::linux::bluez::BluezObjectManager(*system_bus);
  try {
    auto interfaces = manager.GetManagedObjects();
    for (auto& [object, properties] : interfaces) {
      if (properties.count(sdbus::InterfaceName(org::bluez::Adapter1_proxy::INTERFACE_NAME)) == 1) {
        LOG(INFO) << __func__ << ": found bluetooth adapter " << object;
        return std::make_unique<::nearby::linux::BluetoothAdapter>(system_bus,
                                                                   object);
      }
    }
  } catch (const sdbus::Error& e) {
    DBUS_LOG_METHOD_CALL_ERROR(&manager, "GetManagedObjects", e);
  }

  LOG(ERROR) << __func__ << ": couldn't find a bluetooth adapter on this system";
  return nullptr;
}

std::optional<std::string> GetLanguageCode() {
  const char* lang = std::getenv("LANG");
  if (lang == nullptr || *lang == '\0') {
    return std::string("en");
  }

  std::string value(lang);
  size_t dot = value.find('.');
  if (dot != std::string::npos) value.resize(dot);
  size_t underscore = value.find('_');
  if (underscore != std::string::npos) value.resize(underscore);
  if (value.empty()) {
    return std::string("en");
  }
  return value;
}

std::string GetEnvOrDefault(const char* key, std::string fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  return value;
}

FilePath BuildPathFromBase(const std::string& base,
                           std::initializer_list<std::string> components) {
  std::filesystem::path path(base);
  for (const std::string& component : components) {
    path /= component;
  }
  return FilePath(path.string());
}

std::string GetHomeDirectory() {
  return GetEnvOrDefault("HOME", "/tmp");
}

bool HasNonLoopbackInterface() {
  struct ifaddrs* interfaces = nullptr;
  if (getifaddrs(&interfaces) != 0) {
    return false;
  }

  bool connected = false;
  for (struct ifaddrs* current = interfaces; current != nullptr;
       current = current->ifa_next) {
    if (current->ifa_name == nullptr || current->ifa_flags == 0) {
      continue;
    }
    if ((current->ifa_flags & IFF_UP) == 0 ||
        (current->ifa_flags & IFF_LOOPBACK) != 0) {
      continue;
    }
    connected = true;
    break;
  }

  freeifaddrs(interfaces);
  return connected;
}

Json ToJson(const PrivateCertificateData& certificate) {
  return Json{
      {PrivateCertificateData::kVisibility, certificate.visibility},
      {PrivateCertificateData::kNotBefore, certificate.not_before},
      {PrivateCertificateData::kNotAfter, certificate.not_after},
      {PrivateCertificateData::kKeyPair, certificate.key_pair},
      {PrivateCertificateData::kSecretKey, certificate.secret_key},
      {PrivateCertificateData::kMetadataEncryptionKey,
       certificate.metadata_encryption_key},
      {PrivateCertificateData::kId, certificate.id},
      {PrivateCertificateData::kUnencryptedMetadata,
       certificate.unencrypted_metadata_proto},
      {PrivateCertificateData::kConsumedSalts, certificate.consumed_salts},
  };
}

std::optional<PrivateCertificateData> FromJson(const Json& value) {
  if (!value.is_object()) {
    return std::nullopt;
  }

  PrivateCertificateData certificate;
  try {
    certificate.visibility = value.at(PrivateCertificateData::kVisibility);
    certificate.not_before = value.at(PrivateCertificateData::kNotBefore);
    certificate.not_after = value.at(PrivateCertificateData::kNotAfter);
    certificate.key_pair = value.at(PrivateCertificateData::kKeyPair);
    certificate.secret_key = value.at(PrivateCertificateData::kSecretKey);
    certificate.metadata_encryption_key =
        value.at(PrivateCertificateData::kMetadataEncryptionKey);
    certificate.id = value.at(PrivateCertificateData::kId);
    certificate.unencrypted_metadata_proto =
        value.at(PrivateCertificateData::kUnencryptedMetadata);
    certificate.consumed_salts =
        value.at(PrivateCertificateData::kConsumedSalts);
    return certificate;
  } catch (const Json::exception&) {
    return std::nullopt;
  }
}

class FailedSigninAttempt final : public SigninAttempt {
 public:
  std::string Start(
      absl::AnyInvocable<void(AuthStatus, absl::string_view, absl::string_view,
                              const AccountInfo&)>
          callback) override {
    if (callback) {
      std::move(callback)(UNSUPPORTED, "", "", AccountInfo{});
    }
    return {};
  }

  void Close() override {}
};

class LinuxAccountManager final : public AccountManager {
 public:
  std::optional<Account> GetCurrentAccount() override { return std::nullopt; }

  std::unique_ptr<SigninAttempt> Login(absl::string_view client_id,
                                       absl::string_view client_secret)
      override {
    last_client_id_ = std::string(client_id);
    last_client_secret_ = std::string(client_secret);
    return std::make_unique<FailedSigninAttempt>();
  }

  void Logout(absl::AnyInvocable<void(absl::Status)> logout_callback) override {
    if (logout_callback) {
      std::move(logout_callback)(absl::OkStatus());
    }
  }

  bool GetAccessToken(
      absl::AnyInvocable<void(absl::StatusOr<std::string>)> callback)
      override {
    if (!callback) {
      return false;
    }
    std::move(callback)(
        absl::UnavailableError("Linux account integration is not available"));
    return true;
  }

  std::pair<std::string, std::string> GetOAuthClientCredential() override {
    return {last_client_id_, last_client_secret_};
  }

  void AddObserver(Observer* observer) override { observers_.insert(observer); }
  void RemoveObserver(Observer* observer) override {
    observers_.erase(observer);
  }

  void SaveAccountPrefs(absl::string_view user_id, absl::string_view client_id,
                        absl::string_view client_secret) override {
    last_client_id_ = std::string(client_id);
    last_client_secret_ = std::string(client_secret);
  }

 private:
  absl::flat_hash_set<Observer*> observers_;
  std::string last_client_id_;
  std::string last_client_secret_;
};

class SafeLinuxDeviceInfo final : public nearby::DeviceInfo {
 public:
  std::string GetOsDeviceName() const override {
    char hostname[256] = {};
    if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != '\0') {
      return hostname;
    }
    const char* env_hostname = std::getenv("HOSTNAME");
    if (env_hostname != nullptr && *env_hostname != '\0') {
      return env_hostname;
    }
    return "Linux";
  }

  nearby::api::DeviceInfo::DeviceType GetDeviceType() const override {
    return nearby::api::DeviceInfo::DeviceType::kLaptop;
  }

  nearby::api::DeviceInfo::OsType GetOsType() const override {
    return nearby::api::DeviceInfo::OsType::kWindows;
  }

  FilePath GetDownloadPath() const override {
    const char* xdg_download_dir = std::getenv("XDG_DOWNLOAD_DIR");
    if (xdg_download_dir != nullptr && *xdg_download_dir != '\0') {
      return FilePath(std::string(xdg_download_dir));
    }
    return BuildPathFromBase(GetHomeDirectory(), {"Downloads"});
  }

  FilePath GetAppDataPath() const override {
    const std::string config_home =
        GetEnvOrDefault("XDG_CONFIG_HOME",
                        BuildPathFromBase(GetHomeDirectory(), {".config"})
                            .ToString());
    return BuildPathFromBase(config_home, {"Google Nearby"});
  }

  FilePath GetTemporaryPath() const override {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir != nullptr && *runtime_dir != '\0') {
      return BuildPathFromBase(runtime_dir, {"Google Nearby"});
    }
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir != nullptr && *tmpdir != '\0') {
      return BuildPathFromBase(tmpdir, {"Google Nearby"});
    }
    return BuildPathFromBase("/tmp", {"Google Nearby"});
  }

  FilePath GetLogPath() const override {
    const std::string state_home =
        GetEnvOrDefault("XDG_STATE_HOME",
                        BuildPathFromBase(GetHomeDirectory(),
                                          {".local", "state"})
                            .ToString());
    return BuildPathFromBase(state_home, {"Google Nearby", "logs"});
  }

  std::optional<size_t> GetAvailableDiskSpaceInBytes(
      const FilePath& path) const override {
    return Files::GetAvailableDiskSpaceInBytes(path);
  }

  bool IsScreenLocked() const override { return false; }

  void RegisterScreenLockedListener(
      absl::string_view listener_name,
      std::function<void(nearby::api::DeviceInfo::ScreenStatus)> callback)
      override {
    absl::MutexLock lock(&mutex_);
    screen_lock_listeners_[std::string(listener_name)] = std::move(callback);
  }

  void UnregisterScreenLockedListener(absl::string_view listener_name) override {
    absl::MutexLock lock(&mutex_);
    screen_lock_listeners_.erase(std::string(listener_name));
  }

  bool PreventSleep() override { return true; }
  bool AllowSleep() override { return true; }

 private:
  mutable absl::Mutex mutex_;
  absl::flat_hash_map<
      std::string,
      std::function<void(nearby::api::DeviceInfo::ScreenStatus)>>
      screen_lock_listeners_ ABSL_GUARDED_BY(mutex_);
};

class LinuxPreferenceManager final : public PreferenceManager {
 public:
  LinuxPreferenceManager()
      : storage_(nearby::api::ImplementationPlatform::CreatePreferencesManager(
            kPreferencesFile)) {}

  void SetBoolean(absl::string_view key, bool value) override {
    if (storage_ != nullptr && storage_->SetBoolean(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetInteger(absl::string_view key, int value) override {
    if (storage_ != nullptr && storage_->SetInteger(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetInt64(absl::string_view key, int64_t value) override {
    if (storage_ != nullptr && storage_->SetInt64(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetString(absl::string_view key, absl::string_view value) override {
    if (storage_ != nullptr && storage_->SetString(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetTime(absl::string_view key, absl::Time value) override {
    if (storage_ != nullptr && storage_->SetTime(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetBooleanArray(absl::string_view key,
                       absl::Span<const bool> value) override {
    if (storage_ != nullptr && storage_->SetBooleanArray(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetIntegerArray(absl::string_view key,
                       absl::Span<const int> value) override {
    if (storage_ != nullptr && storage_->SetIntegerArray(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetInt64Array(absl::string_view key,
                     absl::Span<const int64_t> value) override {
    if (storage_ != nullptr && storage_->SetInt64Array(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetStringArray(absl::string_view key,
                      absl::Span<const std::string> value) override {
    if (storage_ != nullptr && storage_->SetStringArray(key, value)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetPrivateCertificateArray(
      absl::string_view key, absl::Span<const PrivateCertificateData> value)
      override {
    Json certificates = Json::array();
    for (const PrivateCertificateData& certificate : value) {
      certificates.push_back(ToJson(certificate));
    }
    if (storage_ != nullptr && storage_->Set(key, certificates)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetCertificateExpirationArray(
      absl::string_view key,
      absl::Span<const std::pair<std::string, int64_t>> value) override {
    Json expirations = Json::array();
    for (const auto& [id, expiration] : value) {
      expirations.push_back(Json{{"id", id}, {"expiration", expiration}});
    }
    if (storage_ != nullptr && storage_->Set(key, expirations)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetDictionaryBooleanValue(absl::string_view key,
                                 absl::string_view dictionary_item,
                                 bool value) override {
    SetDictionaryValue(key, dictionary_item, value);
  }

  void SetDictionaryIntegerValue(absl::string_view key,
                                 absl::string_view dictionary_item,
                                 int value) override {
    SetDictionaryValue(key, dictionary_item, value);
  }

  void SetDictionaryInt64Value(absl::string_view key,
                               absl::string_view dictionary_item,
                               int64_t value) override {
    SetDictionaryValue(key, dictionary_item, value);
  }

  void SetDictionaryStringValue(absl::string_view key,
                                absl::string_view dictionary_item,
                                std::string value) override {
    SetDictionaryValue(key, dictionary_item, std::move(value));
  }

  void RemoveDictionaryItem(absl::string_view key,
                            absl::string_view dictionary_item) override {
    if (storage_ == nullptr) {
      return;
    }
    Json dictionary = storage_->Get(key, Json::object());
    if (!dictionary.is_object() || !dictionary.contains(std::string(dictionary_item))) {
      return;
    }
    dictionary.erase(std::string(dictionary_item));
    if (storage_->Set(key, dictionary)) {
      NotifyPreferenceChanged(key);
    }
  }

  void SetSyncConfigValue(absl::string_view binding_id,
                          const SyncConfigPrefs& value) override {
    SetString(absl::StrCat(PrefNames::kSyncConfigPrefix, binding_id),
              value.SerializeAsString());
  }

  void SetSyncBindingValue(const SyncBindingPrefs& value) override {
    SetString(absl::StrCat(PrefNames::kBindingConfigPrefix, kFileSyncBindingName),
              value.SerializeAsString());
  }

  bool GetBoolean(absl::string_view key, bool default_value) const override {
    return storage_ == nullptr ? default_value
                               : storage_->GetBoolean(key, default_value);
  }

  int GetInteger(absl::string_view key, int default_value) const override {
    return storage_ == nullptr ? default_value
                               : storage_->GetInteger(key, default_value);
  }

  int64_t GetInt64(absl::string_view key,
                   int64_t default_value) const override {
    return storage_ == nullptr ? default_value
                               : storage_->GetInt64(key, default_value);
  }

  std::string GetString(absl::string_view key,
                        const std::string& default_value) const override {
    return storage_ == nullptr ? default_value
                               : storage_->GetString(key, default_value);
  }

  absl::Time GetTime(absl::string_view key,
                     absl::Time default_value) const override {
    return storage_ == nullptr ? default_value
                               : storage_->GetTime(key, default_value);
  }

  std::vector<bool> GetBooleanArray(
      absl::string_view key,
      absl::Span<const bool> default_value) const override {
    return storage_ == nullptr ? std::vector<bool>(default_value.begin(),
                                                   default_value.end())
                               : storage_->GetBooleanArray(key, default_value);
  }

  std::vector<int> GetIntegerArray(
      absl::string_view key, absl::Span<const int> default_value) const
      override {
    return storage_ == nullptr ? std::vector<int>(default_value.begin(),
                                                  default_value.end())
                               : storage_->GetIntegerArray(key, default_value);
  }

  std::vector<int64_t> GetInt64Array(
      absl::string_view key, absl::Span<const int64_t> default_value) const
      override {
    return storage_ == nullptr
               ? std::vector<int64_t>(default_value.begin(), default_value.end())
               : storage_->GetInt64Array(key, default_value);
  }

  std::vector<std::string> GetStringArray(
      absl::string_view key,
      absl::Span<const std::string> default_value) const override {
    return storage_ == nullptr
               ? std::vector<std::string>(default_value.begin(),
                                          default_value.end())
               : storage_->GetStringArray(key, default_value);
  }

  std::vector<PrivateCertificateData> GetPrivateCertificateArray(
      absl::string_view key) const override {
    std::vector<PrivateCertificateData> certificates;
    if (storage_ == nullptr) {
      return certificates;
    }

    Json values = storage_->Get(key, Json::array());
    if (!values.is_array()) {
      return certificates;
    }

    for (const Json& value : values) {
      std::optional<PrivateCertificateData> certificate = FromJson(value);
      if (certificate.has_value()) {
        certificates.push_back(*certificate);
      }
    }
    return certificates;
  }

  std::vector<std::pair<std::string, int64_t>> GetCertificateExpirationArray(
      absl::string_view key) const override {
    std::vector<std::pair<std::string, int64_t>> expirations;
    if (storage_ == nullptr) {
      return expirations;
    }

    Json values = storage_->Get(key, Json::array());
    if (!values.is_array()) {
      return expirations;
    }

    for (const Json& value : values) {
      if (!value.is_object()) {
        continue;
      }
      try {
        expirations.emplace_back(value.at("id"), value.at("expiration"));
      } catch (const Json::exception&) {
      }
    }
    return expirations;
  }

  std::optional<bool> GetDictionaryBooleanValue(
      absl::string_view key, absl::string_view dictionary_item) const
      override {
    return GetDictionaryValue<bool>(key, dictionary_item);
  }

  std::optional<int> GetDictionaryIntegerValue(
      absl::string_view key, absl::string_view dictionary_item) const
      override {
    return GetDictionaryValue<int>(key, dictionary_item);
  }

  std::optional<int64_t> GetDictionaryInt64Value(
      absl::string_view key, absl::string_view dictionary_item) const
      override {
    return GetDictionaryValue<int64_t>(key, dictionary_item);
  }

  std::optional<std::string> GetDictionaryStringValue(
      absl::string_view key, absl::string_view dictionary_item) const
      override {
    return GetDictionaryValue<std::string>(key, dictionary_item);
  }

  std::optional<SyncConfigPrefs> GetSyncConfigValue(
      absl::string_view binding_id) const override {
    std::string serialized =
        GetString(absl::StrCat(PrefNames::kSyncConfigPrefix, binding_id), "");
    if (serialized.empty()) {
      return std::nullopt;
    }
    SyncConfigPrefs value;
    if (!value.ParseFromString(serialized)) {
      return std::nullopt;
    }
    return value;
  }

  std::optional<SyncBindingPrefs> GetSyncBindingValue() const override {
    std::string serialized = GetString(
        absl::StrCat(PrefNames::kBindingConfigPrefix, kFileSyncBindingName), "");
    if (serialized.empty()) {
      return std::nullopt;
    }
    SyncBindingPrefs value;
    if (!value.ParseFromString(serialized)) {
      return std::nullopt;
    }
    return value;
  }

  void Remove(absl::string_view key) override {
    if (storage_ == nullptr) {
      return;
    }
    storage_->Remove(key);
    NotifyPreferenceChanged(key);
  }

  void RemoveAllSyncConfigs() override {
    if (storage_ != nullptr) {
      storage_->RemoveKeyPrefix(PrefNames::kSyncConfigPrefix);
    }
  }

  void RemoveAllBindingConfigs() override {
    if (storage_ != nullptr) {
      storage_->RemoveKeyPrefix(PrefNames::kBindingConfigPrefix);
    }
  }

  void AddObserver(
      absl::string_view name,
      std::function<void(absl::string_view pref_name)> observer) override {
    absl::MutexLock lock(&mutex_);
    observers_[std::string(name)] = std::move(observer);
  }

  void RemoveObserver(absl::string_view name) override {
    absl::MutexLock lock(&mutex_);
    observers_.erase(std::string(name));
  }

 private:
  template <typename T>
  void SetDictionaryValue(absl::string_view key,
                          absl::string_view dictionary_item, T value) {
    if (storage_ == nullptr) {
      return;
    }
    Json dictionary = storage_->Get(key, Json::object());
    if (!dictionary.is_object()) {
      return;
    }
    const std::string item(dictionary_item);
    if (dictionary.contains(item) && dictionary[item] == value) {
      return;
    }
    dictionary[item] = value;
    if (storage_->Set(key, dictionary)) {
      NotifyPreferenceChanged(key);
    }
  }

  template <typename T>
  std::optional<T> GetDictionaryValue(
      absl::string_view key, absl::string_view dictionary_item) const {
    if (storage_ == nullptr) {
      return std::nullopt;
    }
    Json dictionary = storage_->Get(key, Json::object());
    if (!dictionary.is_object()) {
      return std::nullopt;
    }

    const std::string item(dictionary_item);
    if (!dictionary.contains(item)) {
      return std::nullopt;
    }

    try {
      return dictionary.at(item).get<T>();
    } catch (const Json::exception&) {
      return std::nullopt;
    }
  }

  void NotifyPreferenceChanged(absl::string_view key) {
    absl::flat_hash_map<std::string, std::function<void(absl::string_view)>>
        observers_copy;
    {
      absl::MutexLock lock(&mutex_);
      observers_copy = observers_;
    }
    for (const auto& [name, observer] : observers_copy) {
      if (observer) {
        observer(key);
      }
    }
  }

  std::unique_ptr<nearby::api::PreferencesManager> storage_;
  mutable absl::Mutex mutex_;
  absl::flat_hash_map<std::string, std::function<void(absl::string_view)>>
      observers_ ABSL_GUARDED_BY(mutex_);
};

class LinuxNetworkMonitor final : public nearby::api::NetworkMonitor {
 public:
  LinuxNetworkMonitor(std::function<void(bool)> lan_connected_callback,
                      std::function<void(bool)> internet_connected_callback)
      : nearby::api::NetworkMonitor(std::move(lan_connected_callback),
                                    std::move(internet_connected_callback)) {
    const bool lan_connected = IsLanConnected();
    const bool internet_connected = IsInternetConnected();
    if (lan_connected_callback_) {
      lan_connected_callback_(lan_connected);
    }
    if (internet_connected_callback_) {
      internet_connected_callback_(internet_connected);
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
    if (uname(&info) != 0) {
      return {};
    }
    return info.release;
  }

  std::string GetOsArchitecture() override {
    struct utsname info;
    if (uname(&info) != 0) {
      return {};
    }
    return info.machine;
  }

  std::string GetOsLanguage() override {
    return GetLanguageCode().value_or("en");
  }

  std::string GetProcessorManufacturer() override { return "Unknown"; }
  std::string GetProcessorName() override { return "Unknown"; }
  std::list<DriverInfo> GetBluetoothDriverInfos() override { return {}; }
  std::list<DriverInfo> GetNetworkDriverInfos() override { return {}; }
  void GetBatteryUsageReport(const FilePath& save_path) override {}
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
    preference_manager_.SetInt64(kAppActivePref, absl::ToUnixNanos(absl::Now()));
    return true;
  }

 private:
  PreferenceManager& preference_manager_;
};

class LinuxFastInitBleBeacon final : public nearby::api::FastInitBleBeacon {
 public:
  void SerializeToByteArray() override {
    SetAdDataByteArray(BuildFastInitAdvertisingData(*this));
  }

  void ParseFromByteArray() override {
    const auto data = GetAdDataByteArray();
    if (data[0] != nearby::api::FastInitBleBeacon::kFastInitServiceUuid[0] ||
        data[1] != nearby::api::FastInitBleBeacon::kFastInitServiceUuid[1]) {
      return;
    }

    const uint8_t metadata = data[5];
    SetVersion(static_cast<FastInitVersion>((metadata >> 5) & 0x07));
    SetType(static_cast<FastInitType>((metadata >> 2) & 0x07));
    SetUwbSupported((metadata & 0x02) != 0);
    SetSenderCertSupported((metadata & 0x01) != 0);
    SetAdjustedTxPower(static_cast<int8_t>(data[6]));

    std::array<uint8_t, kUwbMetadataSize> uwb_metadata{};
    uwb_metadata[0] = data[7];
    SetUwbMetadata(uwb_metadata);

    std::array<uint8_t, kUwbAddressSize> uwb_address{};
    for (size_t i = 0; i < uwb_address.size(); ++i) {
      uwb_address[i] = data[8 + i];
    }
    SetUwbAddress(uwb_address);

    std::array<uint8_t, kSaltSize> salt{};
    salt[0] = data[16];
    SetSalt(salt);

    std::array<uint8_t, kSecretIdHashSize> secret_id_hash{};
    for (size_t i = 0; i < secret_id_hash.size(); ++i) {
      secret_id_hash[i] = data[17 + i];
    }
    SetSecretIdHash(secret_id_hash);

    const uint8_t flags = data[25];
    SetRequireBtAdvertising((flags & 0x80) != 0);
    SetSelfOnlyAdvertising((flags & 0x40) != 0);
  }
};

class LinuxFastInitiationManager final
    : public nearby::api::FastInitiationManager {
 public:
  explicit LinuxFastInitiationManager(nearby::api::FastInitBleBeacon& beacon)
      : beacon_(beacon), adapter_(CreateFastInitBluetoothAdapter()) {
    if (adapter_ != nullptr) {
      adv_manager_ =
          std::make_unique<::nearby::linux::bluez::LEAdvertisementManager>(
              *adapter_->GetConnection(), *adapter_);
    }
  }

  void StartAdvertising(
      nearby::api::FastInitBleBeacon::FastInitType type,
      std::function<void()> callback,
      std::function<void(nearby::api::FastInitiationManager::Error)>
          error_callback) override {
    absl::MutexLock lock(&mutex_);
    if (advertisement_ != nullptr) {
      if (error_callback) {
        error_callback(nearby::api::FastInitiationManager::Error::kResourceInUse);
      }
      return;
    }

    if (adapter_ == nullptr || !adapter_->IsEnabled()) {
      if (error_callback) {
        error_callback(
            nearby::api::FastInitiationManager::Error::kBluetoothRadioUnavailable);
      }
      return;
    }

    if (adv_manager_ == nullptr) {
      if (error_callback) {
        error_callback(
            nearby::api::FastInitiationManager::Error::kHardwareNotSupported);
      }
      return;
    }

    auto fast_init_uuid = GetFastInitUuid();
    if (!fast_init_uuid.has_value()) {
      if (error_callback) {
        error_callback(nearby::api::FastInitiationManager::Error::kUnknown);
      }
      return;
    }

    beacon_.SetVersion(nearby::api::FastInitBleBeacon::FastInitVersion::kV1);
    beacon_.SetType(type);
    beacon_.SetUwbSupported(false);
    beacon_.SetSenderCertSupported(false);
    beacon_.SetAdjustedTxPower(0);
    beacon_.SetUwbMetadata({});
    beacon_.SetUwbAddress({});

    const std::string seed = GetFastInitSeed(type, adapter_.get());
    beacon_.SetSalt(BuildFastInitSalt(seed));
    beacon_.SetSecretIdHash(BuildFastInitSecretIdHash(seed));
    beacon_.SetRequireBtAdvertising(false);
    beacon_.SetSelfOnlyAdvertising(
        type == nearby::api::FastInitBleBeacon::FastInitType::kSilent);
    beacon_.SerializeToByteArray();

    const auto ad_data = beacon_.GetAdDataByteArray();
    nearby::api::ble::BleAdvertisementData advertising_data;
    advertising_data.is_extended_advertisement = true;
    advertising_data.service_data.insert(
        {*fast_init_uuid,
         nearby::ByteArray(reinterpret_cast<const char*>(ad_data.data() + 2),
                           ad_data.size() - 2)});

    nearby::api::ble::AdvertiseParameters advertising_parameters{
        .tx_power_level = nearby::api::ble::TxPowerLevel::kHigh,
        .is_connectable = false,
    };
    advertisement_ =
        ::nearby::linux::bluez::LEAdvertisement::CreateLEAdvertisement(
            *adapter_->GetConnection(), advertising_data,
            advertising_parameters);

    try {
      adv_manager_->RegisterAdvertisementSync(advertisement_->getObject().getObjectPath(), {});
    } catch (const sdbus::Error& e) {
      advertisement_.reset();
      if (error_callback) {
        if (e.getName() == "org.bluez.Error.AlreadyExists") {
          error_callback(
              nearby::api::FastInitiationManager::Error::kResourceInUse);
        } else if (e.getName() == "org.bluez.Error.NotPermitted") {
          error_callback(nearby::api::FastInitiationManager::Error::kDisabledByUser);
        } else {
          error_callback(nearby::api::FastInitiationManager::Error::kUnknown);
        }
      }
      return;
    }

    if (callback) {
      callback();
    }
  }

  void StopAdvertising(std::function<void()> callback) override {
    absl::MutexLock lock(&mutex_);
    if (advertisement_ != nullptr && adv_manager_ != nullptr) {
      try {
        adv_manager_->UnregisterAdvertisementSync(advertisement_->getObject().getObjectPath());
      } catch (const sdbus::Error& e) {
        DBUS_LOG_METHOD_CALL_ERROR(adv_manager_.get(), "UnregisterAdvertisementSync",
                                   e);
      }
      advertisement_.reset();
    }
    if (callback) {
      callback();
    }
  }

  void StartScanning(
      std::function<void()> devices_discovered_callback,
      std::function<void()> devices_not_discovered_callback,
      std::function<void(nearby::api::FastInitiationManager::Error)>
          error_callback) override {
    if (error_callback) {
      error_callback(
          nearby::api::FastInitiationManager::Error::kHardwareNotSupported);
    }
  }

  void StopScanning(std::function<void()> callback) override {
    if (callback) {
      callback();
    }
  }

  bool IsAdvertising() override {
    absl::MutexLock lock(&mutex_);
    return advertisement_ != nullptr;
  }
  bool IsScanning() override { return false; }

 private:
  nearby::api::FastInitBleBeacon& beacon_;
  std::unique_ptr<::nearby::linux::BluetoothAdapter> adapter_;
  std::unique_ptr<::nearby::linux::bluez::LEAdvertisementManager> adv_manager_;
  absl::Mutex mutex_;
  std::unique_ptr<::nearby::linux::bluez::LEAdvertisement> advertisement_
      ABSL_GUARDED_BY(mutex_);
};

class LinuxBluetoothAdapter final
    : public nearby::sharing::api::BluetoothAdapter {
 public:
  bool IsPresent() const override { return GetAddress().IsSet(); }

  bool IsPowered() const override {
    return adapter_.IsValid() && adapter_.IsEnabled();
  }

  bool IsLowEnergySupported() const override { return true; }
  bool IsScanOffloadSupported() const override { return false; }
  bool IsAdvertisementOffloadSupported() const override { return false; }
  bool IsExtendedAdvertisingSupported() const override { return false; }
  bool IsPeripheralRoleSupported() const override { return true; }

  PermissionStatus GetOsPermissionStatus() const override {
    return PermissionStatus::kAllowed;
  }

  void SetPowered(bool powered, std::function<void()> success_callback,
                  std::function<void()> error_callback) override {
    if (!adapter_.IsValid()) {
      if (error_callback) {
        error_callback();
      }
      return;
    }
    const bool success = adapter_.SetStatus(
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
    if (!adapter_.IsValid()) {
      return std::nullopt;
    }
    std::string name = adapter_.GetName();
    if (name.empty()) {
      return std::nullopt;
    }
    return name;
  }

  MacAddress GetAddress() const override {
    if (!adapter_.IsValid()) {
      return {};
    }
    return adapter_.GetAddress();
  }

  void AddObserver(Observer* observer) override { observers_.insert(observer); }
  void RemoveObserver(Observer* observer) override {
    observers_.erase(observer);
  }
  bool HasObserver(Observer* observer) override {
    return observers_.contains(observer);
  }

 private:
  nearby::BluetoothAdapter adapter_;
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
      absl::AnyInvocable<void(
          bool, std::unique_ptr<std::vector<PublicCertificate>>) &&>
          callback) override {
    auto entries = std::make_unique<std::vector<PublicCertificate>>();
    {
      absl::MutexLock lock(&mutex_);
      for (const auto& [id, certificate] : entries_) {
        entries->push_back(certificate);
      }
    }
    if (callback) {
      std::move(callback)(true, std::move(entries));
    }
  }

  void LoadCertificate(
      absl::string_view id,
      absl::AnyInvocable<void(bool, std::unique_ptr<PublicCertificate>) &&>
          callback) override {
    std::unique_ptr<PublicCertificate> certificate;
    {
      absl::MutexLock lock(&mutex_);
      auto it = entries_.find(std::string(id));
      if (it != entries_.end()) {
        certificate = std::make_unique<PublicCertificate>(it->second);
      }
    }
    if (callback) {
      std::move(callback)(certificate != nullptr, std::move(certificate));
    }
  }

  void AddCertificates(absl::Span<const PublicCertificate> certificates,
                       absl::AnyInvocable<void(bool) &&> callback) override {
    {
      absl::MutexLock lock(&mutex_);
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
      absl::MutexLock lock(&mutex_);
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
      absl::MutexLock lock(&mutex_);
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

LinuxSharingPlatform::LinuxSharingPlatform() { Initialize({}); }

LinuxSharingPlatform::LinuxSharingPlatform(std::string device_name_override) {
  Initialize(std::move(device_name_override));
}

LinuxSharingPlatform::~LinuxSharingPlatform() = default;

void LinuxSharingPlatform::Initialize(std::string device_name_override) {
  preference_manager_ = std::make_unique<LinuxPreferenceManager>();
  account_manager_ = std::make_unique<LinuxAccountManager>();
  bluetooth_adapter_ = std::make_unique<LinuxBluetoothAdapter>();
  fast_init_ble_beacon_ = std::make_unique<LinuxFastInitBleBeacon>();
  fast_initiation_manager_ =
      std::make_unique<LinuxFastInitiationManager>(*fast_init_ble_beacon_);
  default_task_runner_ = std::make_unique<TaskRunnerImpl>(1);
  device_info_ = std::make_unique<SafeLinuxDeviceInfo>();

  if (!device_name_override.empty()) {
    preference_manager_->SetString(PrefNames::kDeviceName, device_name_override);
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
  return std::make_unique<LinuxNetworkMonitor>(
      std::move(lan_connected_callback), std::move(internet_connected_callback));
}

nearby::sharing::api::BluetoothAdapter&
LinuxSharingPlatform::GetBluetoothAdapter() {
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
  return std::make_unique<LinuxSystemInfo>();
}

std::unique_ptr<nearby::api::AppInfo> LinuxSharingPlatform::CreateAppInfo() {
  return std::make_unique<LinuxAppInfo>(*preference_manager_);
}

nearby::sharing::api::PreferenceManager&
LinuxSharingPlatform::GetPreferenceManager() {
  return *preference_manager_;
}

AccountManager& LinuxSharingPlatform::GetAccountManager() {
  return *account_manager_;
}

TaskRunner& LinuxSharingPlatform::GetDefaultTaskRunner() {
  return *default_task_runner_;
}

nearby::DeviceInfo& LinuxSharingPlatform::GetDeviceInfo() {
  return *device_info_;
}

std::unique_ptr<nearby::sharing::api::PublicCertificateDatabase>
LinuxSharingPlatform::CreatePublicCertificateDatabase(
    const FilePath& database_path) {
  return std::make_unique<LinuxPublicCertificateDatabase>();
}

bool LinuxSharingPlatform::UpdateFileOriginMetadata(
    std::vector<FilePath>& file_paths) {
  return true;
}

}  // namespace nearby::sharing::linux
