#include <sdbus-c++/Types.h>
#include "internal/platform/implementation/linux/ble_v2.h"

#include "internal/platform/implementation/linux/ble_v2_advertisement.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {
BleV2Medium::BleV2Medium(api::BluetoothAdapter &adapter) : adapter_(dynamic_cast<linux::BluetoothAdapter *>(&adapter))
{
  auto system_bus = linux::getSystemBusConnection();
  bluez_object_manager_ = std::make_unique<bluez::BluezObjectManager>(*system_bus);
}
bool BleV2Medium::StartAdvertising(const api::ble_v2::BleAdvertisementData &advertising_data,
                                   api::ble_v2::AdvertiseParameters advertise_set_parameters)
{
  if (!adapter_->IsEnabled())
  {
    NEARBY_LOGS(WARNING) << "BLE cannot start advertising because the "
                            "bluetooth adapter is not enabled.";
    return false;
  }

  if (advertising_data.service_data.empty())
  {
    NEARBY_LOGS(WARNING) << "BLE cannot start to advertise due to invalid service data.";
    return false;
  }

  auto system_bus = adapter_->GetConnection();
  auto le_advertisement = linux::LEAdvertisement::CreateLEAdvertisement(
    *system_bus,  advertising_data, advertise_set_parameters);

  if (!le_advertisement)
  {
    NEARBY_LOGS(WARNING) << "Some error occurred while creating LEAdvertisement object.";
    return false;
  }
  auto le_advertisement_manager =
    std::make_unique<linux::LEAdvertisementManager>(*system_bus, *adapter_);
  try
  {
    le_advertisement_manager->RegisterAdvertisement(le_advertisement->getObjectPath(), {});
  }
  catch (const sdbus::Error& e)
  {
    DBUS_LOG_METHOD_CALL_ERROR(le_advertisement_manager, "RegisterAdvertisement", e);
    return false;
  }
  return true;
}

// dummy overrides
std::unique_ptr<api::ble_v2::BleMedium::AdvertisingSession>
BleV2Medium::StartAdvertising(const api::ble_v2::BleAdvertisementData &advertising_data,
                              api::ble_v2::AdvertiseParameters advertise_set_parameters, AdvertisingCallback callback)
{
  return nullptr;
}
bool BleV2Medium::StartScanning(const Uuid &service_uuid, api::ble_v2::TxPowerLevel tx_power_level,
                                ScanCallback callback)
{
  return false;
}
bool BleV2Medium::StopAdvertising() { return false; }
bool BleV2Medium::IsExtendedAdvertisementsAvailable() { return true; }
std::unique_ptr<api::ble_v2::GattServer>
BleV2Medium::StartGattServer(api::ble_v2::ServerGattConnectionCallback callback)
{
  return nullptr;
}
bool BleV2Medium::StopScanning() { return false; }
std::unique_ptr<api::ble_v2::GattClient>
BleV2Medium::ConnectToGattServer(api::ble_v2::BlePeripheral &peripheral, api::ble_v2::TxPowerLevel tx_power_level,
                                 api::ble_v2::ClientGattConnectionCallback callback)
{
  return nullptr;
}
std::unique_ptr<api::ble_v2::BleServerSocket> BleV2Medium::OpenServerSocket(const std::string &service_id)
{

  LOG(INFO) << "OpenServerSocket is called";

  auto server_socket = std::make_unique<BleV2ServerSocket>(*adapter_);


  return server_socket;
}
bool BleV2Medium::GetRemotePeripheral(const std::string &mac_address, GetRemotePeripheralCallback callback)
{
  return false;
}
bool BleV2Medium::GetRemotePeripheral(api::ble_v2::BlePeripheral::UniqueId id, GetRemotePeripheralCallback callback)
{
  return false;
}
std::unique_ptr<api::ble_v2::BleSocket> BleV2Medium::Connect(const std::string &service_id,
                                                             api::ble_v2::TxPowerLevel tx_power_level,
                                                             api::ble_v2::BlePeripheral &peripheral,
                                                             CancellationFlag *cancellation_flag)
{
  return nullptr;
}
std::unique_ptr<api::ble_v2::BleMedium::ScanningSession>
BleV2Medium::StartScanning(const Uuid &service_uuid, api::ble_v2::TxPowerLevel tx_power_level,
                           ScanningCallback callback)
{
  return nullptr;
}
// end of dummy overrides

BleV2ServerSocket::BleV2ServerSocket(api::BluetoothAdapter &adapter) :
    adapter_(dynamic_cast<linux::BluetoothAdapter *>(&adapter))
{
}

std::unique_ptr<api::ble_v2::BleSocket> BleV2ServerSocket::Accept()
{
  absl::MutexLock lock(&mutex_);
  LOG(INFO) << __func__ << ": Accept is called.";

  while (!closed_ && pending_sockets_.empty())
  {
    cond_.Wait(&mutex_);
  }
  if (closed_)
    return nullptr;

  linux::BleV2Socket ble_socket = pending_sockets_.front();
  pending_sockets_.pop_front();

  LOG(INFO) << __func__ << ": Accepted a remote connection.";
  return std::make_unique<linux::BleV2Socket>(ble_socket);
}

Exception BleV2ServerSocket::Close()
{
  // TODO(b/271031645): implement BLE socket using weave
  absl::MutexLock lock(&mutex_);
  LOG(INFO) << __func__ << ": Close is called.";

  if (closed_)
  {
    return {Exception::kSuccess};
  }

  closed_ = true;
  cond_.SignalAll();

  return {Exception::kSuccess};
}


} // namespace linux
} // namespace nearby
