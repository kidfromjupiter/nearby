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

#ifndef PLATFORM_IMPL_LINUX_BLUEZ_ADVERTISEMENT_MONITOR_MANAGER_H_
#define PLATFORM_IMPL_LINUX_BLUEZ_ADVERTISEMENT_MONITOR_MANAGER_H_

#include <optional>

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/ProxyInterfaces.h>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/advertisement_monitor_manager_client.h"

namespace nearby {
namespace linux {
namespace bluez {
class AdvertisementMonitorManager final
    : public sdbus::ProxyInterfaces<
          org::bluez::AdvertisementMonitorManager1_proxy> {
 public:
  using ReplyCallback = absl::AnyInvocable<void(std::optional<sdbus::Error>)>;

 private:
  friend std::unique_ptr<AdvertisementMonitorManager>
  std::make_unique<AdvertisementMonitorManager>(
      sdbus::IConnection &, const ::nearby::linux::BluetoothAdapter &);
  AdvertisementMonitorManager(
      sdbus::IConnection &system_bus,
      const ::nearby::linux::BluetoothAdapter &adapter)
      : ProxyInterfaces(system_bus, sdbus::ServiceName("org.bluez"),
                        adapter.GetObjectPath()) {
    registerProxy();
  }

 public:
  AdvertisementMonitorManager(const AdvertisementMonitorManager &) = delete;
  AdvertisementMonitorManager(AdvertisementMonitorManager &&) = delete;
  AdvertisementMonitorManager &operator=(const AdvertisementMonitorManager &) =
      delete;
  AdvertisementMonitorManager &operator=(AdvertisementMonitorManager &&) =
      delete;
  ~AdvertisementMonitorManager() { unregisterProxy(); }

  void SetRegisterMonitorReplyCallback(ReplyCallback callback) {
    absl::MutexLock lock(&callbacks_mutex_);
    register_monitor_reply_callback_ = std::move(callback);
  }

  void SetUnregisterMonitorReplyCallback(ReplyCallback callback) {
    absl::MutexLock lock(&callbacks_mutex_);
    unregister_monitor_reply_callback_ = std::move(callback);
  }

  static std::unique_ptr<AdvertisementMonitorManager>
  DiscoverAdvertisementMonitorManager(
      sdbus::IConnection &system_bus,
      const ::nearby::linux::BluetoothAdapter &adapter) {
    bluez::BluezObjectManager manager(system_bus);
    std::map<sdbus::ObjectPath,
             std::map<sdbus::InterfaceName, std::map<sdbus::PropertyName, sdbus::Variant>>>
        objects;
    try {
      objects = manager.GetManagedObjects();
    } catch (const sdbus::Error &e) {
      DBUS_LOG_METHOD_CALL_ERROR(&manager, "GetManagedObjects", e);
      return nullptr;
    }
    if (objects.count(adapter.GetObjectPath()) == 0) {
      LOG(ERROR) << __func__ << ": Adapter object no longer exists "
                         << adapter.GetObjectPath();
      return nullptr;
    }

    if (objects[adapter.GetObjectPath()].count(
            sdbus::InterfaceName(org::bluez::AdvertisementMonitorManager1_proxy::INTERFACE_NAME)) ==
        0) {
      LOG(ERROR)
          << __func__ << ": Adapter " << adapter.GetObjectPath()
          << " doesn't provide "
          << org::bluez::AdvertisementMonitorManager1_proxy::INTERFACE_NAME;
      return nullptr;
    }

    return std::make_unique<AdvertisementMonitorManager>(system_bus, adapter);
  }

 protected:
  void onRegisterMonitorReply(std::optional<sdbus::Error> error) override {
    std::optional<ReplyCallback> callback;
    {
      absl::MutexLock lock(&callbacks_mutex_);
      callback = std::move(register_monitor_reply_callback_);
      register_monitor_reply_callback_.reset();
    }
    if (callback.has_value()) {
      (*callback)(std::move(error));
    }
  }

  void onUnregisterMonitorReply(std::optional<sdbus::Error> error) override {
    std::optional<ReplyCallback> callback;
    {
      absl::MutexLock lock(&callbacks_mutex_);
      callback = std::move(unregister_monitor_reply_callback_);
      unregister_monitor_reply_callback_.reset();
    }
    if (callback.has_value()) {
      (*callback)(std::move(error));
    }
  }

 private:
  absl::Mutex callbacks_mutex_;
  std::optional<ReplyCallback> register_monitor_reply_callback_
      ABSL_GUARDED_BY(callbacks_mutex_);
  std::optional<ReplyCallback> unregister_monitor_reply_callback_
      ABSL_GUARDED_BY(callbacks_mutex_);
};
}  // namespace bluez
}  // namespace linux
}  // namespace nearby

#endif
