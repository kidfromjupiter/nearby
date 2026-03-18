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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_SYNC_MANAGER_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_SYNC_MANAGER_H_

#include <optional>

#include "absl/strings/string_view.h"
#include "sharing/internal/api/preference_manager.h"

namespace nearby::sharing {

class SyncManager {
 public:
  explicit SyncManager(api::PreferenceManager* preference_manager)
      : preference_manager_(preference_manager) {}

  bool IsFileSyncBinding(absl::string_view binding_id) const {
    static_cast<void>(binding_id);
    return false;
  }

  std::optional<nearby::sharing::sync::SyncConfigPrefs> GetSyncConfig(
      absl::string_view binding_id) const {
    static_cast<void>(binding_id);
    return std::nullopt;
  }

 private:
  api::PreferenceManager* preference_manager_;
};

}  // namespace nearby::sharing

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_SYNC_MANAGER_H_
