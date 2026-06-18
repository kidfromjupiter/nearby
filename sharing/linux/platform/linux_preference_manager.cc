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

#include "sharing/linux/platform/linux_preference_manager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "internal/platform/implementation/platform.h"
#include "location/nearby/sharing/lib/sync/sync_binding_prefs.pb.h"
#include "nlohmann/json.hpp"
#include "sharing/internal/api/private_certificate_data.h"
#include "sharing/internal/public/pref_names.h"

namespace nearby::sharing::linux::internal {
namespace {

using Json = nlohmann::json;
using ::nearby::sharing::api::PreferenceManager;
using ::nearby::sharing::api::PrivateCertificateData;

constexpr absl::string_view kPreferencesFile = "nearby_sharing_linux.json";
constexpr absl::string_view kFileSyncBindingName = "FileSync";

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
    if (!dictionary.is_object()) {
      return;
    }
    dictionary.erase(std::string(dictionary_item));
    if (storage_->Set(key, dictionary)) {
      NotifyPreferenceChanged(key);
    }
  }
  void SetSyncBindingValue(
      const nearby::sharing::sync::SyncBindingPrefs& value) override {
    std::string serialized;
    if (value.SerializeToString(&serialized)) {
      SetString(absl::StrCat(PrefNames::kBindingConfigPrefix,
                             kFileSyncBindingName),
                serialized);
    }
  }

  bool GetBoolean(absl::string_view key, bool default_value) const override {
    return storage_ != nullptr ? storage_->GetBoolean(key, default_value)
                               : default_value;
  }
  int GetInteger(absl::string_view key, int default_value) const override {
    return storage_ != nullptr ? storage_->GetInteger(key, default_value)
                               : default_value;
  }
  int64_t GetInt64(absl::string_view key,
                   int64_t default_value) const override {
    return storage_ != nullptr ? storage_->GetInt64(key, default_value)
                               : default_value;
  }
  std::string GetString(absl::string_view key,
                        const std::string& default_value) const override {
    return storage_ != nullptr ? storage_->GetString(key, default_value)
                               : default_value;
  }
  absl::Time GetTime(absl::string_view key,
                     absl::Time default_value) const override {
    return storage_ != nullptr ? storage_->GetTime(key, default_value)
                               : default_value;
  }
  std::vector<bool> GetBooleanArray(
      absl::string_view key,
      absl::Span<const bool> default_value) const override {
    return storage_ != nullptr ? storage_->GetBooleanArray(key, default_value)
                               : std::vector<bool>(default_value.begin(),
                                                   default_value.end());
  }
  std::vector<int> GetIntegerArray(
      absl::string_view key,
      absl::Span<const int> default_value) const override {
    return storage_ != nullptr ? storage_->GetIntegerArray(key, default_value)
                               : std::vector<int>(default_value.begin(),
                                                  default_value.end());
  }
  std::vector<int64_t> GetInt64Array(
      absl::string_view key,
      absl::Span<const int64_t> default_value) const override {
    return storage_ != nullptr ? storage_->GetInt64Array(key, default_value)
                               : std::vector<int64_t>(default_value.begin(),
                                                      default_value.end());
  }
  std::vector<std::string> GetStringArray(
      absl::string_view key,
      absl::Span<const std::string> default_value) const override {
    return storage_ != nullptr ? storage_->GetStringArray(key, default_value)
                               : std::vector<std::string>(default_value.begin(),
                                                          default_value.end());
  }
  std::vector<PrivateCertificateData> GetPrivateCertificateArray(
      absl::string_view key) const override {
    std::vector<PrivateCertificateData> result;
    if (storage_ == nullptr) {
      return result;
    }
    Json certificates = storage_->Get(key, Json::array());
    if (!certificates.is_array()) {
      return result;
    }
    for (const Json& certificate_json : certificates) {
      std::optional<PrivateCertificateData> certificate =
          FromJson(certificate_json);
      if (certificate.has_value()) {
        result.push_back(*certificate);
      }
    }
    return result;
  }
  std::vector<std::pair<std::string, int64_t>>
  GetCertificateExpirationArray(absl::string_view key) const override {
    std::vector<std::pair<std::string, int64_t>> result;
    if (storage_ == nullptr) {
      return result;
    }
    Json expirations = storage_->Get(key, Json::array());
    if (!expirations.is_array()) {
      return result;
    }
    for (const Json& item : expirations) {
      if (!item.is_object()) {
        continue;
      }
      try {
        result.emplace_back(item.at("id").get<std::string>(),
                            item.at("expiration").get<int64_t>());
      } catch (const Json::exception&) {
      }
    }
    return result;
  }
  std::optional<bool> GetDictionaryBooleanValue(
      absl::string_view key, absl::string_view dictionary_item) const override {
    return GetDictionaryValue<bool>(key, dictionary_item);
  }
  std::optional<int> GetDictionaryIntegerValue(
      absl::string_view key, absl::string_view dictionary_item) const override {
    return GetDictionaryValue<int>(key, dictionary_item);
  }
  std::optional<int64_t> GetDictionaryInt64Value(
      absl::string_view key, absl::string_view dictionary_item) const override {
    return GetDictionaryValue<int64_t>(key, dictionary_item);
  }
  std::optional<std::string> GetDictionaryStringValue(
      absl::string_view key, absl::string_view dictionary_item) const override {
    return GetDictionaryValue<std::string>(key, dictionary_item);
  }
  std::optional<nearby::sharing::sync::SyncBindingPrefs> GetSyncBindingValue()
      const override {
    std::string serialized =
        GetString(absl::StrCat(PrefNames::kBindingConfigPrefix,
                               kFileSyncBindingName),
                  "");
    if (serialized.empty()) {
      return std::nullopt;
    }
    nearby::sharing::sync::SyncBindingPrefs value;
    if (!value.ParseFromString(serialized)) {
      return std::nullopt;
    }
    return value;
  }
  void Remove(absl::string_view key) override {
    if (storage_ != nullptr) {
      storage_->Remove(key);
      NotifyPreferenceChanged(key);
    }
  }
  void RemoveAllBindingConfigs() override {
    if (storage_ != nullptr &&
        storage_->RemoveKeyPrefix(PrefNames::kBindingConfigPrefix)) {
      NotifyPreferenceChanged(PrefNames::kBindingConfigPrefix);
    }
  }
  void AddObserver(
      absl::string_view name,
      std::function<void(absl::string_view pref_name)> observer) override {
    observers_[std::string(name)] = std::move(observer);
  }
  void RemoveObserver(absl::string_view name) override {
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
      dictionary = Json::object();
    }
    dictionary[std::string(dictionary_item)] = std::move(value);
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
    auto it = dictionary.find(std::string(dictionary_item));
    if (it == dictionary.end()) {
      return std::nullopt;
    }
    try {
      return it->get<T>();
    } catch (const Json::exception&) {
      return std::nullopt;
    }
  }

  void NotifyPreferenceChanged(absl::string_view key) {
    for (const auto& [name, observer] : observers_) {
      static_cast<void>(name);
      if (observer) {
        observer(key);
      }
    }
  }

  std::unique_ptr<nearby::api::PreferencesManager> storage_;
  absl::flat_hash_map<std::string,
                      std::function<void(absl::string_view pref_name)>>
      observers_;
};

}  // namespace

std::unique_ptr<api::PreferenceManager> CreateLinuxPreferenceManager() {
  return std::make_unique<LinuxPreferenceManager>();
}

}  // namespace nearby::sharing::linux::internal
