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

#include "internal/platform/implementation/linux/ble_gatt_server.h"
#include "absl/strings/substitute.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/implementation/linux/bluez_gatt_characteristic_server.h"
#include "internal/platform/implementation/linux/bluez_gatt_manager.h"
#include "internal/platform/implementation/linux/bluez_gatt_service_server.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/implementation/linux/generated/dbus/bluez/gatt_service_server.h"
#include "internal/platform/uuid.h"

namespace nearby {
namespace linux {
namespace {

void LogAsyncGattManagerError(const bluez::GattManager& manager,
                              const char* method_name,
                              const sdbus::Error& error) {
  LOG(ERROR) << method_name << ": Got error '" << error.getName()
             << "' with message '" << error.getMessage()
             << "' while calling " << method_name << " on object "
             << manager.getProxy().getObjectPath();
}

}  // namespace

absl::optional<api::ble::GattCharacteristic>
GattServer::CreateCharacteristic(
    const Uuid& service_uuid, const Uuid& characteristic_uuid,
    api::ble::GattCharacteristic::Permission permission,
    api::ble::GattCharacteristic::Property property) {
  absl::MutexLock lock(&services_mutex_);
  if (services_.count(service_uuid) == 1) {
    if (services_[service_uuid]->AddCharacteristic(
            service_uuid, characteristic_uuid, permission, property)) {
      api::ble::GattCharacteristic characteristic{
          characteristic_uuid, service_uuid, permission, property};
      return characteristic;
    }
    return std::nullopt;
  }

  auto count = services_.size();
  auto service = std::make_unique<bluez::GattServiceServer>(
      system_bus_, count, service_uuid, server_cb_, devices_);
  try {
    service->emitInterfacesAddedSignal(
        {sdbus::InterfaceName(org::bluez::GattService1_adaptor::INTERFACE_NAME)});
  } catch (const sdbus::Error& e) {
    LOG(ERROR)
        << __func__
        << ": error emitting InterfacesAdded signal for object path "
        << service->getObject().getObjectPath() << " with name '" << e.getName()
        << "' and message '" << e.getMessage() << "'";
    return std::nullopt;
  }
  auto profile = std::make_unique<bluez::GattProfile> (system_bus_, bluez::gatt_profile_object_path(
    std::string(service_uuid)), std::string(service_uuid));
  profile -> emitInterfacesAddedSignal();

  if (service->AddCharacteristic(service_uuid, characteristic_uuid, permission,
                                 property)) {
    services_.insert({service_uuid, std::move(service)});
    {
      absl::MutexLock profile_lock(&profiles_mutex_);
      gatt_profiles_.insert({service_uuid, std::move(profile)});
    }

    bool should_register_application = false;
    {
      absl::MutexLock registration_lock(&registration_mutex_);
      should_register_application =
          !gatt_application_registered_ && !gatt_application_register_pending_;
      if (should_register_application) {
        gatt_application_register_pending_ = true;
      }
    }

    if (should_register_application) {
      try {
        LOG(INFO) << __func__
                  << ": Registering GATT application root "
                  << gatt_service_root_object_manager->getObject().getObjectPath()
                  << " with characteristic_uuid: "
                  << std::string(characteristic_uuid)
                  << " and service_uuid: " << std::string(service_uuid);
        auto call = gatt_manager_->RegisterApplication(
            gatt_service_root_object_manager->getObject().getObjectPath(), {});
        absl::MutexLock registration_lock(&registration_mutex_);
        register_application_call_ = std::move(call);
      } catch (const sdbus::Error& e) {
        {
          absl::MutexLock registration_lock(&registration_mutex_);
          gatt_application_register_pending_ = false;
          register_application_call_.reset();
        }
        services_.erase(service_uuid);
        {
          absl::MutexLock profile_lock(&profiles_mutex_);
          gatt_profiles_.erase(service_uuid);
        }
        LOG(ERROR)
            << __func__
            << ": error calling RegisterApplication for GattManager with object path "
            << gatt_manager_->getProxy().getObjectPath() << " with name '"
            << e.getName() << "' and message '" << e.getMessage() << "'";
        return std::nullopt;
      }
    }

    api::ble::GattCharacteristic characteristic{
        characteristic_uuid, service_uuid, permission, property};
    return characteristic;
  }

  return std::nullopt;
}

bool GattServer::UpdateCharacteristic(
    const api::ble::GattCharacteristic& characteristic,
    const nearby::ByteArray& value) {
  std::shared_ptr<bluez::GattCharacteristicServer> chr = nullptr;
  {
    absl::ReaderMutexLock lock(&services_mutex_);
    if (services_.count(characteristic.service_uuid) == 0) {
      LOG(ERROR) << __func__ << ": GATT Service "
                         << std::string{characteristic.service_uuid}
                         << " doesn't exist";
      return false;
    }
    chr = services_[characteristic.service_uuid]->GetCharacteristic(
        characteristic.uuid);
  }
  if (chr == nullptr) {
    LOG(ERROR) << __func__ << ": Characteristic "
                       << std::string{characteristic.uuid}
                       << " does not exist under service "
                       << std::string{characteristic.service_uuid};
    return false;
  }
  assert(chr != nullptr);
  chr->Update(value);
  return true;
}

absl::Status GattServer::NotifyCharacteristicChanged(
    const api::ble::GattCharacteristic& characteristic, bool confirm,
    const ByteArray& new_value) {
  std::shared_ptr<bluez::GattCharacteristicServer> chr = nullptr;
  {
    absl::ReaderMutexLock lock(&services_mutex_);
    if (services_.count(characteristic.service_uuid) == 0) {
      return absl::NotFoundError(
          absl::Substitute("Service $0 doesn't exist",
                           std::string{characteristic.service_uuid}));
    }
    chr = services_[characteristic.service_uuid]->GetCharacteristic(
        characteristic.uuid);
  }
  if (chr == nullptr) {
    return absl::NotFoundError(
        absl::Substitute("characteristic $0 doesn't exist under service $1",
                         std::string{characteristic.uuid},
                         std::string{characteristic.service_uuid}));
  }

  return chr->NotifyChanged(confirm, new_value);
}

void GattServer::OnRegisterApplicationReply(std::optional<sdbus::Error> error) {
  {
    absl::MutexLock lock(&registration_mutex_);
    gatt_application_register_pending_ = false;
    register_application_call_.reset();
    gatt_application_registered_ = !error.has_value() || !error->isValid();
  }

  if (error.has_value() && error->isValid()) {
    LogAsyncGattManagerError(*gatt_manager_, "RegisterApplication", *error);
  }
}

void GattServer::OnUnregisterApplicationReply(
    std::optional<sdbus::Error> error) {
  {
    absl::MutexLock lock(&registration_mutex_);
    gatt_application_unregister_pending_ = false;
    unregister_application_call_.reset();
  }

  if (error.has_value() && error->isValid()) {
    LogAsyncGattManagerError(*gatt_manager_, "UnregisterApplication", *error);
  }
}

void GattServer::Stop() {
  sdbus::ObjectPath app_path =
      gatt_service_root_object_manager->getObject().getObjectPath();
  bool should_unregister = false;

  {
    absl::MutexLock registration_lock(&registration_mutex_);
    if (register_application_call_.has_value() &&
        register_application_call_->isPending()) {
      register_application_call_->cancel();
    }
    register_application_call_.reset();
    gatt_application_register_pending_ = false;

    should_unregister = gatt_application_registered_ &&
                        !gatt_application_unregister_pending_;
    if (should_unregister) {
      gatt_application_registered_ = false;
      gatt_application_unregister_pending_ = true;
    }
  }

  {
    absl::MutexLock lock(&services_mutex_);
    for (auto& [uuid, service] : services_) {
      LOG(INFO) << __func__ << ": Unregistering service "
                << service->getObject().getObjectPath();
    }
  }

  if (should_unregister) {
    try {
      auto call = gatt_manager_->UnregisterApplication(app_path);
      absl::MutexLock registration_lock(&registration_mutex_);
      unregister_application_call_ = std::move(call);
    } catch (const sdbus::Error& e) {
      {
        absl::MutexLock registration_lock(&registration_mutex_);
        gatt_application_unregister_pending_ = false;
        unregister_application_call_.reset();
      }
      DBUS_LOG_METHOD_CALL_ERROR(gatt_manager_.get(), "UnregisterApplication",
                                 e);
    }
  }

  {
    absl::MutexLock lock(&services_mutex_);
    services_.clear();
  }
  {
    absl::MutexLock profile_lock(&profiles_mutex_);
    gatt_profiles_.clear();
  }
}

}  // namespace linux
}  // namespace nearby
