#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/dbus.h"
#include "sharing/linux/nearby_fast_init_ble_beacon.h"
#include "sharing/linux/nearby_fast_init_manager.h"
#include <thread>
#include <chrono>

// For local testing only. Android logs show that our fast init beacon is
// properly detected and they start advertising.

int main() {
  auto system_bus = ::nearby::linux::getSystemBusConnection();
  auto manager = ::nearby::linux::bluez::BluezObjectManager(*system_bus);
  std::shared_ptr<nearby::linux::BluetoothAdapter> adapter;
  auto interfaces = manager.GetManagedObjects();
  for (auto& [object, properties] : interfaces) {
    if (properties.count(sdbus::InterfaceName(
            org::bluez::Adapter1_proxy::INTERFACE_NAME)) == 1) {
      LOG(INFO) << __func__ << ": found bluetooth adapter " << object;
      adapter = std::make_shared<::nearby::linux::BluetoothAdapter>(system_bus,
                                                                    object);
    }
  }
  auto beacon = ::nearby::sharing::linux::LinuxFastInitBleBeacon();

  try {
    auto interfaces = manager.GetManagedObjects();
    for (auto& [object, properties] : interfaces) {
      if (properties.count(sdbus::InterfaceName(
              org::bluez::Adapter1_proxy::INTERFACE_NAME)) == 1) {
        LOG(INFO) << __func__ << ": found bluetooth adapter " << object;
        adapter = std::make_shared<::nearby::linux::BluetoothAdapter>(
            system_bus, object);
      }
    }
  } catch (const sdbus::Error& e) {
    DBUS_LOG_METHOD_CALL_ERROR(&manager, "GetManagedObjects", e);
  }

  auto fast_init_manager =
      ::nearby::sharing::linux::LinuxFastInitiationManager(beacon, adapter);

  fast_init_manager.StartAdvertising(
      nearby::api::FastInitBleBeacon::FastInitType::kNotify, []() { return; },
      [](nearby::api::FastInitiationManager::Error e) { return; });

  std::this_thread::sleep_for(std::chrono::seconds(30));
  return 0;
}
