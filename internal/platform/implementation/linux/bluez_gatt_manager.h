// Copyright 2023 Google LLC
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

#ifndef PLATFORM_IMPL_LINUX_API_BLUEZ_GATT_MANAGER_H_
#define PLATFORM_IMPL_LINUX_API_BLUEZ_GATT_MANAGER_H_

#include <optional>

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/ProxyInterfaces.h>
#include <sdbus-c++/Types.h>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/gatt_manager_client.h"
namespace nearby {
namespace linux {
namespace bluez {
class GattManager
    : public sdbus::ProxyInterfaces<org::bluez::GattManager1_proxy> {
 public:
  using ReplyCallback = absl::AnyInvocable<void(std::optional<sdbus::Error>)>;

  GattManager(const GattManager &) = delete;
  GattManager(GattManager &&) = delete;
  GattManager &operator=(const GattManager &) = delete;
  GattManager &operator=(GattManager &&) = delete;

  GattManager(sdbus::IConnection &system_bus,
              sdbus::ObjectPath adapter_object_path)
      : ProxyInterfaces(system_bus, sdbus::ServiceName("org.bluez"),
                        std::move(adapter_object_path)) {
    registerProxy();
  }
  ~GattManager() { unregisterProxy(); }

  void SetRegisterApplicationReplyCallback(ReplyCallback callback) {
    absl::MutexLock lock(&callbacks_mutex_);
    register_application_reply_callback_ = std::move(callback);
  }

  void SetUnregisterApplicationReplyCallback(ReplyCallback callback) {
    absl::MutexLock lock(&callbacks_mutex_);
    unregister_application_reply_callback_ = std::move(callback);
  }

 protected:
  void onRegisterApplicationReply(std::optional<sdbus::Error> error) override {
    absl::MutexLock lock(&callbacks_mutex_);
    if (register_application_reply_callback_.has_value()) {
      (*register_application_reply_callback_)(std::move(error));
    }
  }

  void onUnregisterApplicationReply(std::optional<sdbus::Error> error) override {
    absl::MutexLock lock(&callbacks_mutex_);
    if (unregister_application_reply_callback_.has_value()) {
      (*unregister_application_reply_callback_)(std::move(error));
    }
  }

 private:
  absl::Mutex callbacks_mutex_;
  std::optional<ReplyCallback> register_application_reply_callback_
      ABSL_GUARDED_BY(callbacks_mutex_);
  std::optional<ReplyCallback> unregister_application_reply_callback_
      ABSL_GUARDED_BY(callbacks_mutex_);
};
}  // namespace bluez
}  // namespace linux
}  // namespace nearby
#endif
