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

#include "sharing/linux/platform/linux_account_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace nearby::sharing::linux::internal {
namespace {

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
    static_cast<void>(user_id);
    last_client_id_ = std::string(client_id);
    last_client_secret_ = std::string(client_secret);
  }

 private:
  absl::flat_hash_set<Observer*> observers_;
  std::string last_client_id_;
  std::string last_client_secret_;
};

}  // namespace

std::unique_ptr<AccountManager> CreateLinuxAccountManager() {
  return std::make_unique<LinuxAccountManager>();
}

}  // namespace nearby::sharing::linux::internal
