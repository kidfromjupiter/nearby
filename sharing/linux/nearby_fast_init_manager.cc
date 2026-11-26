#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include "internal/platform/implementation/ble.h"
#include "sharing/internal/api/fast_init_ble_beacon.h"
#include "sharing/internal/api/fast_initiation_manager.h"

#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluetooth_devices.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/bluez_advertisement_monitor.h"
#include "internal/platform/implementation/linux/bluez_advertisement_monitor_manager.h"
#include "internal/platform/implementation/linux/bluez_le_advertisement.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "internal/platform/uuid.h"
#include "sharing/linux/nearby_fast_init_manager.h"

namespace nearby {
namespace sharing {
namespace linux {

namespace {

constexpr char kFastInitMonitorRootPath[] =
    "/com/google/nearby/sharing/fast_init/monitor";
constexpr char kFastInitMonitorPath[] =
    "/com/google/nearby/sharing/fast_init/monitor/fe2c";
constexpr size_t kFastInitServiceDataSize =
    nearby::api::FastInitBleBeacon::kAdvertiseDataTotalSize -
    nearby::api::FastInitBleBeacon::kFastInitServiceUuidSize;

nearby::api::FastInitiationManager::Error MapBluezError(
    const std::string& error_name) {
  if (error_name == "org.bluez.Error.AlreadyExists" ||
      error_name == "org.bluez.Error.InProgress") {
    return nearby::api::FastInitiationManager::Error::kResourceInUse;
  }
  if (error_name == "org.bluez.Error.NotPermitted" ||
      error_name == "org.bluez.Error.NotAuthorized") {
    return nearby::api::FastInitiationManager::Error::kDisabledByUser;
  }
  if (error_name == "org.bluez.Error.NotSupported") {
    return nearby::api::FastInitiationManager::Error::kHardwareNotSupported;
  }
  return nearby::api::FastInitiationManager::Error::kUnknown;
}

}  // namespace

LinuxFastInitiationManager::~LinuxFastInitiationManager() {
  StopScanning(nullptr);
  StopAdvertising(nullptr);
}

void LinuxFastInitiationManager::StartAdvertising(
    nearby::api::FastInitBleBeacon::FastInitType type,
    std::function<void()> callback,
    std::function<void(nearby::api::FastInitiationManager::Error)>
        error_callback) {
  absl::MutexLock lock(mutex_);
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
  absl::MutexLock lock(mutex_);
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
void LinuxFastInitiationManager::StartScanning(
    std::function<void()> devices_discovered_callback,
    std::function<void()> devices_not_discovered_callback,
    std::function<void(nearby::api::FastInitiationManager::Error)>
        error_callback) {
  std::optional<nearby::api::FastInitiationManager::Error> error;
  {
    absl::MutexLock operation_lock(scan_operation_mutex_);
    error = StartScanningInternal(std::move(devices_discovered_callback),
                                  std::move(devices_not_discovered_callback));
  }
  if (error.has_value() && error_callback) {
    error_callback(*error);
  }
}

std::optional<nearby::api::FastInitiationManager::Error>
LinuxFastInitiationManager::StartScanningInternal(
    std::function<void()> devices_discovered_callback,
    std::function<void()> devices_not_discovered_callback) {
  bool already_scanning = false;
  {
    absl::MutexLock lock(mutex_);
    already_scanning = is_scanning_;
  }
  if (already_scanning) {
    return nearby::api::FastInitiationManager::Error::kResourceInUse;
  }

  if (adapter_ == nullptr || !adapter_->IsEnabled()) {
    return nearby::api::FastInitiationManager::Error::
        kBluetoothRadioUnavailable;
  }

  auto fast_init_uuid = Uuid::FromString(kFastInitServiceUuid);
  if (!fast_init_uuid.has_value()) {
    return nearby::api::FastInitiationManager::Error::kUnknown;
  }

  auto connection = adapter_->GetConnection();
  auto monitor_manager = ::nearby::linux::bluez::AdvertisementMonitorManager::
      DiscoverAdvertisementMonitorManager(*connection, *adapter_);
  if (monitor_manager == nullptr) {
    return nearby::api::FastInitiationManager::Error::kHardwareNotSupported;
  }

  try {
    const std::vector<std::string> supported_types =
        monitor_manager->SupportedMonitorTypes();
    if (std::find(supported_types.begin(), supported_types.end(),
                  "or_patterns") == supported_types.end()) {
      return nearby::api::FastInitiationManager::Error::kHardwareNotSupported;
    }
  } catch (const sdbus::Error& error) {
    return MapBluezError(error.getName());
  }

  uint64_t generation;
  try {
    {
      absl::MutexLock lock(mutex_);
      is_scanning_ = true;
      generation = ++scan_generation_;
      devices_discovered_callback_ = std::move(devices_discovered_callback);
      devices_not_discovered_callback_ =
          std::move(devices_not_discovered_callback);
      discovered_peripherals_.clear();
    }
    scan_devices_ = ::nearby::linux::GetSharedBluetoothDevices(
        connection, adapter_->GetObjectPath());
    scan_root_object_manager_ =
        std::make_unique<::nearby::linux::RootObjectManager>(
            *connection, sdbus::ObjectPath(kFastInitMonitorRootPath));
    scan_monitor_ =
        std::make_unique<::nearby::linux::bluez::AdvertisementMonitor>(
            *connection, sdbus::ObjectPath(kFastInitMonitorPath),
            *fast_init_uuid, nearby::api::ble::TxPowerLevel::kLow,
            "or_patterns", scan_devices_,
            nearby::api::ble::BleMedium::ScanningCallback{
                .advertisement_found_cb =
                    [this, generation](
                        nearby::api::ble::BlePeripheral::UniqueId peripheral_id,
                        nearby::api::ble::BleAdvertisementData data) {
                      OnAdvertisementFound(generation, peripheral_id,
                                           std::move(data));
                    },
                .advertisement_lost_cb =
                    [this,
                     generation](nearby::api::ble::BlePeripheral::UniqueId id) {
                      OnAdvertisementLost(generation, id);
                    },
            });
    scan_monitor_manager_ = std::move(monitor_manager);
  } catch (const sdbus::Error& error) {
    {
      absl::MutexLock lock(mutex_);
      is_scanning_ = false;
      devices_discovered_callback_ = nullptr;
      devices_not_discovered_callback_ = nullptr;
    }
    scan_monitor_.reset();
    scan_root_object_manager_.reset();
    scan_devices_.reset();
    return MapBluezError(error.getName());
  }

  absl::Notification registration_complete;
  std::string registration_error_name;
  scan_monitor_manager_->SetRegisterMonitorReplyCallback(
      [&registration_complete,
       &registration_error_name](std::optional<sdbus::Error> error) {
        if (error.has_value() && error->isValid()) {
          registration_error_name = error->getName();
        }
        registration_complete.Notify();
      });
  try {
    scan_monitor_manager_->RegisterMonitor(
        scan_root_object_manager_->getObject().getObjectPath());
    registration_complete.WaitForNotification();
  } catch (const sdbus::Error& error) {
    registration_error_name = error.getName();
  }

  if (!registration_error_name.empty()) {
    {
      absl::MutexLock lock(mutex_);
      is_scanning_ = false;
      ++scan_generation_;
      devices_discovered_callback_ = nullptr;
      devices_not_discovered_callback_ = nullptr;
      discovered_peripherals_.clear();
    }
    scan_monitor_.reset();
    scan_root_object_manager_.reset();
    scan_monitor_manager_.reset();
    scan_devices_.reset();
    return MapBluezError(registration_error_name);
  }

  try {
    auto& bluez_adapter = adapter_->GetBluezAdapterObject();
    if (!bluez_adapter.Discovering()) {
      std::map<std::string, sdbus::Variant> filter;
      filter["Transport"] = sdbus::Variant("le");
      filter["DuplicateData"] = sdbus::Variant(true);
      bluez_adapter.SetDiscoveryFilter(filter);
      bluez_adapter.StartDiscovery();
      absl::MutexLock lock(mutex_);
      owns_bluez_discovery_ = true;
    }
  } catch (const sdbus::Error& error) {
    // Release the scan application before reporting a failed start.
    StopScanningInternal();
    return MapBluezError(error.getName());
  }
  return std::nullopt;
}

void LinuxFastInitiationManager::StopScanning(std::function<void()> callback) {
  {
    absl::MutexLock operation_lock(scan_operation_mutex_);
    StopScanningInternal();
  }
  if (callback) {
    callback();
  }
}

void LinuxFastInitiationManager::StopScanningInternal() {
  bool owns_discovery = false;
  {
    absl::MutexLock lock(mutex_);
    if (!is_scanning_) {
      return;
    }
    is_scanning_ = false;
    owns_discovery = owns_bluez_discovery_;
    owns_bluez_discovery_ = false;
    ++scan_generation_;
    devices_discovered_callback_ = nullptr;
    devices_not_discovered_callback_ = nullptr;
    discovered_peripherals_.clear();
  }

  if (owns_discovery && adapter_ != nullptr) {
    try {
      adapter_->GetBluezAdapterObject().StopDiscovery();
    } catch (const sdbus::Error& error) {
      DBUS_LOG_METHOD_CALL_ERROR(&adapter_->GetBluezAdapterObject(),
                                 "StopDiscovery", error);
    }
  }

  if (scan_monitor_manager_ != nullptr &&
      scan_root_object_manager_ != nullptr) {
    absl::Notification unregistration_complete;
    scan_monitor_manager_->SetUnregisterMonitorReplyCallback(
        [&unregistration_complete](std::optional<sdbus::Error>) {
          unregistration_complete.Notify();
        });
    try {
      scan_monitor_manager_->UnregisterMonitor(
          scan_root_object_manager_->getObject().getObjectPath());
      unregistration_complete.WaitForNotification();
    } catch (const sdbus::Error& error) {
      DBUS_LOG_METHOD_CALL_ERROR(scan_monitor_manager_.get(),
                                 "UnregisterMonitor", error);
    }
  }

  scan_monitor_.reset();
  scan_root_object_manager_.reset();
  scan_monitor_manager_.reset();
  scan_devices_.reset();
}

bool LinuxFastInitiationManager::IsScanning() {
  absl::MutexLock lock(mutex_);
  return is_scanning_;
}

void LinuxFastInitiationManager::OnAdvertisementFound(
    uint64_t scan_generation,
    nearby::api::ble::BlePeripheral::UniqueId peripheral_id,
    nearby::api::ble::BleAdvertisementData advertisement_data) {
  if (!IsFastInitAdvertisement(advertisement_data)) {
    return;
  }

  std::function<void()> callback;
  {
    absl::MutexLock lock(mutex_);
    if (!is_scanning_ || scan_generation != scan_generation_) {
      return;
    }
    auto [unused, inserted] = discovered_peripherals_.insert(peripheral_id);
    if (inserted && discovered_peripherals_.size() == 1) {
      callback = devices_discovered_callback_;
    }
  }
  if (callback) {
    callback();
  }
}

void LinuxFastInitiationManager::OnAdvertisementLost(
    uint64_t scan_generation,
    nearby::api::ble::BlePeripheral::UniqueId peripheral_id) {
  std::function<void()> callback;
  {
    absl::MutexLock lock(mutex_);
    if (!is_scanning_ || scan_generation != scan_generation_ ||
        discovered_peripherals_.erase(peripheral_id) == 0) {
      return;
    }
    if (discovered_peripherals_.empty()) {
      callback = devices_not_discovered_callback_;
    }
  }
  if (callback) {
    callback();
  }
}

bool LinuxFastInitiationManager::IsFastInitAdvertisement(
    const nearby::api::ble::BleAdvertisementData& advertisement_data) const {
  auto fast_init_uuid = Uuid::FromString(kFastInitServiceUuid);
  if (!fast_init_uuid.has_value()) {
    return false;
  }
  auto service_data = advertisement_data.service_data.find(*fast_init_uuid);
  if (service_data == advertisement_data.service_data.end() ||
      service_data->second.size() != kFastInitServiceDataSize) {
    return false;
  }

  const auto bytes = service_data->second.AsStringView();
  for (size_t i = 0; i < nearby::api::FastInitBleBeacon::kFastInitModelIdSize;
       ++i) {
    if (static_cast<uint8_t>(bytes[i]) !=
        nearby::api::FastInitBleBeacon::kFastInitModelId[i]) {
      return false;
    }
  }

  const uint8_t metadata = static_cast<uint8_t>(
      bytes[nearby::api::FastInitBleBeacon::kFastInitModelIdSize]);
  const uint8_t version = (metadata >> 5) & 0x07;
  const uint8_t type = (metadata >> 2) & 0x07;
  return version == static_cast<uint8_t>(
                        nearby::api::FastInitBleBeacon::FastInitVersion::kV1) &&
         type <= static_cast<uint8_t>(
                     nearby::api::FastInitBleBeacon::FastInitType::kSilent);
}
}  // namespace linux
}  // namespace sharing
}  // namespace nearby
