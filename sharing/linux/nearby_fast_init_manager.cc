#include <functional>

#include "sharing/internal/api/fast_init_ble_beacon.h"
#include "sharing/internal/api/fast_initiation_manager.h"

#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "sharing/linux/nearby_fast_init_manager.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/bluez_le_advertisement.h"

namespace nearby {
namespace sharing {
namespace linux {

void LinuxFastInitiationManager::StartAdvertising(
    nearby::api::FastInitBleBeacon::FastInitType type,
    std::function<void()> callback,
    std::function<void(nearby::api::FastInitiationManager::Error)>
        error_callback) {
  absl::MutexLock lock(&mutex_);
  if (advertisement_ != nullptr) {
    if (error_callback) {
      error_callback(nearby::api::FastInitiationManager::Error::kResourceInUse);
    }
    return;
  }

  if (adapter_ == nullptr || !adapter_->IsEnabled()) {
    if (error_callback) {
      error_callback(nearby::api::FastInitiationManager::Error::
                         kBluetoothRadioUnavailable);
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

  auto fast_init_uuid = Uuid::FromString(kFastInitServiceUuid);
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
  beacon_.SetAdjustedTxPower(
      45);  // TODO: make this set the real value from adapter
  beacon_.SetUwbMetadata({});
  beacon_.SetUwbAddress({});

  beacon_.SetSalt({});
  beacon_.SetSecretIdHash({});
  beacon_.SetRequireBtAdvertising(true);
  beacon_.SetSelfOnlyAdvertising(false);
  beacon_.SerializeToByteArray();

  const auto ad_data = beacon_.GetAdDataByteArray();
  nearby::api::ble::BleAdvertisementData advertising_data;
  advertising_data.is_extended_advertisement = false;
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
          *adapter_->GetConnection(), advertising_data, advertising_parameters);

  try {
    adv_manager_->RegisterAdvertisementSync(
        advertisement_->getObject().getObjectPath(), {});
  } catch (const sdbus::Error& e) {
    advertisement_.reset();
    if (error_callback) {
      if (e.getName() == "org.bluez.Error.AlreadyExists") {
        error_callback(
            nearby::api::FastInitiationManager::Error::kResourceInUse);
      } else if (e.getName() == "org.bluez.Error.NotPermitted") {
        error_callback(
            nearby::api::FastInitiationManager::Error::kDisabledByUser);
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

void LinuxFastInitiationManager::StopAdvertising(
    std::function<void()> callback) {
  absl::MutexLock lock(&mutex_);
  if (advertisement_ != nullptr && adv_manager_ != nullptr) {
    try {
      adv_manager_->UnregisterAdvertisementSync(
          advertisement_->getObject().getObjectPath());
    } catch (const sdbus::Error& e) {
      DBUS_LOG_METHOD_CALL_ERROR(adv_manager_.get(),
                                 "UnregisterAdvertisementSync", e);
    }
    advertisement_.reset();
  }
  if (callback) {
    callback();
  }
}
void StopScanning(std::function<void()> callback) {
  if (callback) {
    callback();
  }
}
void LinuxFastInitiationManager::StartScanning(
    std::function<void()> devices_discovered_callback,
    std::function<void()> devices_not_discovered_callback,
    std::function<void(nearby::api::FastInitiationManager::Error)>
        error_callback) {
  if (error_callback) {
    error_callback(
        nearby::api::FastInitiationManager::Error::kHardwareNotSupported);
  }
}
}  // namespace linux
}  // namespace sharing
}  // namespace nearby

