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

#ifndef PLATFORM_IMPL_LINUX_API_BLUEZ_BLE_ADVERTISEMENT_H_
#define PLATFORM_IMPL_LINUX_API_BLUEZ_BLE_ADVERTISEMENT_H_

#include <future>
#include <sdbus-c++/AdaptorInterfaces.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/ProxyInterfaces.h>
#include <sdbus-c++/StandardInterfaces.h>
#include <sdbus-c++/Types.h>

#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/le_advertisement_manager_client.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/le_advertisement_server.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {
namespace bluez {
class LEAdvertisement final
    : public sdbus::AdaptorInterfaces<org::bluez::LEAdvertisement1_adaptor,
                                      sdbus::ObjectManager_adaptor> {
 public:
  LEAdvertisement(const LEAdvertisement&) = delete;
  LEAdvertisement(LEAdvertisement&&) = delete;
  LEAdvertisement& operator=(const LEAdvertisement&) = delete;
  LEAdvertisement& operator=(LEAdvertisement&&) = delete;

  LEAdvertisement(sdbus::IConnection& system_bus, sdbus::ObjectPath path,
                  const api::ble::BleAdvertisementData& advertising_data,
                  api::ble::AdvertiseParameters advertise_set_parameters);

  static std::unique_ptr<LEAdvertisement> CreateLEAdvertisement(
      sdbus::IConnection& system_bus,
      const api::ble::BleAdvertisementData& advertising_data,
      api::ble::AdvertiseParameters advertising_parameters) {
    static std::atomic<size_t> adv_count = 0;
    auto object_path = bluez::ble_advertisement_path(adv_count++);
    return std::make_unique<LEAdvertisement>(
        system_bus, object_path, advertising_data, advertising_parameters);
  }
  ~LEAdvertisement() { unregisterAdaptor(); }

 private:
  // Methods
  void Release() override {
    LOG(INFO) << __func__
                      << ": LE Advertisement released: "
                      << getObject().getObjectPath();
  }

  // Properties  
  std::string Type() override { return "peripheral"; }
  std::vector<std::string> ServiceUUIDs() override { return service_uuids_; }
  std::map<std::string, sdbus::Variant> ManufacturerData() override {
    return {};
  }
  std::vector<std::string> SolicitUUIDs() override { return {}; }
  std::map<std::string, sdbus::Variant> ServiceData() override {
    return service_data_;
  }
  //std::map<std::string, sdbus::Variant> ScanResponseServiceData() override {
  //  return {};
  //}
  std::vector<std::string> Includes() override {
    return {};
  }
  std::string LocalName() override { return {}; }
  uint16_t Duration() override { return 0; }
  uint16_t Timeout() override { return 0; }
  // Windows seems to hardcode the scan interval to 118.125 milliseconds, so
  // lets just replicate that.
  uint32_t MinInterval() override { return 118; }
  uint32_t MaxInterval() override { return 119; }
  int16_t TxPower() override {
    return bluez::TxPowerLevelDbm(advertise_set_parameters_.tx_power_level);
  };

  bool is_extended_advertisement_;
  std::vector<std::string> service_uuids_;
  std::map<std::string, sdbus::Variant> service_data_;
  api::ble::AdvertiseParameters advertise_set_parameters_;
};

class LEAdvertisementManager final
    : public sdbus::ProxyInterfaces<org::bluez::LEAdvertisingManager1_proxy> {
 public:
  LEAdvertisementManager(sdbus::IConnection& system_bus,
                         BluetoothAdapter& adapter)
      : ProxyInterfaces(system_bus, sdbus::ServiceName("org.bluez"),
                        adapter.GetObjectPath()) {
    registerProxy();
  }
  ~LEAdvertisementManager() { unregisterProxy(); }

  LEAdvertisementManager(const LEAdvertisementManager&) = delete;
  LEAdvertisementManager(LEAdvertisementManager&&) = delete;
  LEAdvertisementManager& operator=(const LEAdvertisementManager&) = delete;
  LEAdvertisementManager& operator=(LEAdvertisementManager&&) = delete;

  // Synchronous wrapper for RegisterAdvertisement
  void RegisterAdvertisementSync(const sdbus::ObjectPath& advertisement, 
                                  const std::map<std::string, sdbus::Variant>& options) {
    std::promise<std::optional<sdbus::Error>> promise;
    auto future = promise.get_future();
    
    {
      absl::MutexLock lock(&pending_ops_mutex_);
      pending_register_op_ = std::move(promise);
    }
    
    RegisterAdvertisement(advertisement, options);
    
    auto error = future.get();
    if (error) {
      throw *error;
    }
  }

  // Synchronous wrapper for UnregisterAdvertisement  
  void UnregisterAdvertisementSync(const sdbus::ObjectPath& service) {
    std::promise<std::optional<sdbus::Error>> promise;
    auto future = promise.get_future();
    
    {
      absl::MutexLock lock(&pending_ops_mutex_);
      pending_unregister_op_ = std::move(promise);
    }
    
    UnregisterAdvertisement(service);
    
    auto error = future.get();
    if (error) {
      throw *error;
    }
  }

 protected:
  void onRegisterAdvertisementReply(std::optional<sdbus::Error> error) override {
    absl::MutexLock lock(&pending_ops_mutex_);
    if (pending_register_op_) {
      pending_register_op_->set_value(std::move(error));
      pending_register_op_.reset();
    }
  }

  void onUnregisterAdvertisementReply(std::optional<sdbus::Error> error) override {
    absl::MutexLock lock(&pending_ops_mutex_);
    if (pending_unregister_op_) {
      pending_unregister_op_->set_value(std::move(error));
      pending_unregister_op_.reset();
    }
  }

 private:
  absl::Mutex pending_ops_mutex_;
  std::optional<std::promise<std::optional<sdbus::Error>>> pending_register_op_ 
      ABSL_GUARDED_BY(pending_ops_mutex_);
  std::optional<std::promise<std::optional<sdbus::Error>>> pending_unregister_op_
      ABSL_GUARDED_BY(pending_ops_mutex_);
};
}  // namespace bluez
}  // namespace linux
}  // namespace nearby

#endif
