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

#ifndef LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_MANAGER_H_
#define LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_MANAGER_H_

#include <optional>
#include <string>
#include <utility>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "internal/base/file_path.h"
#include "location/nearby/sharing/lib/rpc/sharing_rpc_client.h"
#include "location/nearby/sharing/lib/sync/sync_binding_prefs.pb.h"
#include "sharing/internal/api/preference_manager.h"

namespace nearby::sharing {

class SyncManager {
 public:
  SyncManager(api::IdentityRpcClient* identity_client,
              api::PreferenceManager* preference_manager)
      : identity_client_(identity_client), preference_manager_(preference_manager) {}

  explicit SyncManager(api::PreferenceManager* preference_manager)
      : SyncManager(nullptr, preference_manager) {}

  void AsyncInitiateSyncBinding(
      absl::AnyInvocable<void(absl::StatusOr<std::string>)> callback) {
    static_cast<void>(identity_client_);
    if (callback) {
      std::move(callback)(absl::UnavailableError(
          "Sync binding RPC is not available in this build"));
    }
  }

  void AddSyncBinding(const sync::SyncBinding& binding) {
    if (preference_manager_ == nullptr) {
      return;
    }
    sync::SyncBindingPrefs prefs =
        preference_manager_->GetSyncBindingValue().value_or(
            sync::SyncBindingPrefs());
    sync::SyncBinding* stored = prefs.add_sync_bindings();
    *stored = binding;
    preference_manager_->SetSyncBindingValue(prefs);
  }

  std::optional<sync::SyncBinding> GetSyncBinding(
      absl::string_view binding_id) const {
    if (preference_manager_ == nullptr) {
      return std::nullopt;
    }
    std::optional<sync::SyncBindingPrefs> prefs =
        preference_manager_->GetSyncBindingValue();
    if (!prefs.has_value()) {
      return std::nullopt;
    }
    for (const sync::SyncBinding& binding : prefs->sync_bindings()) {
      if (binding.binding_id() == binding_id) {
        return binding;
      }
    }
    return std::nullopt;
  }

  bool IsFileSyncBinding(absl::string_view binding_id) const {
    return GetSyncBinding(binding_id).has_value();
  }

  std::optional<sync::SyncConfigPrefs> GetSyncConfig(
      absl::string_view binding_id) const {
    static_cast<void>(binding_id);
    return std::nullopt;
  }

  absl::StatusOr<FilePath> UpdateSyncBindingDestinationDirectory(
      absl::string_view binding_id, FilePath save_path) {
    if (preference_manager_ == nullptr) {
      return absl::FailedPreconditionError("Preference manager is unavailable");
    }
    sync::SyncBindingPrefs prefs =
        preference_manager_->GetSyncBindingValue().value_or(
            sync::SyncBindingPrefs());
    std::optional<FilePath> original_path;
    sync::SyncBindingPrefs updated_prefs;
    for (const sync::SyncBinding& binding : prefs.sync_bindings()) {
      sync::SyncBinding* updated_binding = updated_prefs.add_sync_bindings();
      *updated_binding = binding;
      if (binding.binding_id() == binding_id) {
        original_path = FilePath(binding.destination_directory());
        updated_binding->set_destination_directory(save_path.ToString());
      }
    }
    if (!original_path.has_value()) {
      return absl::NotFoundError(
          absl::StrCat("Sync binding not found: ", binding_id));
    }
    preference_manager_->SetSyncBindingValue(updated_prefs);
    return *original_path;
  }

 private:
  api::IdentityRpcClient* identity_client_;
  api::PreferenceManager* preference_manager_;
};

}  // namespace nearby::sharing

#endif  // LOCATION_NEARBY_SHARING_LIB_SYNC_SYNC_MANAGER_H_

