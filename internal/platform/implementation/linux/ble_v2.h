#include "internal/platform/implementation/ble_v2.h"

namespace nearby {
namespace linux {
class BleV2Peripheral : public api::ble_v2::BlePeripheral
{
  std::string GetAddress() const override;
  UniqueId GetUniqueId() const override;

};

class GattClient: public api::ble_v2::GattClient
{
  bool DiscoverServiceAndCharacteristics(const Uuid& service_uuid, const std::vector<Uuid>& characteristic_uuids) override;

  absl::optional<api::ble_v2::GattCharacteristic>
  GetCharacteristic(const Uuid& service_uuid, const Uuid& characteristic_uuid) override;

  absl::optional<std::string>
  ReadCharacteristic(const api::ble_v2::GattCharacteristic& characteristic) override;

  bool WriteCharacteristic(
    const api::ble_v2::GattCharacteristic& characteristic,
    absl::string_view value, WriteType type) override;

  bool SetCharacteristicSubscription(
    const api::ble_v2::GattCharacteristic& characteristic,
    bool enable, absl::AnyInvocable<void(absl::string_view value)> on_characteristic_changed_cb)
  override;

  void Disconnect() override;
};

class GattServer : public api::ble_v2::GattServer
{
  api::ble_v2::BlePeripheral& GetBlePeripheral() override;

  absl::optional<api::ble_v2::GattCharacteristic> CreateCharacteristic(
    const Uuid& service_uuid, const Uuid& characteristic_uuid,
    api::ble_v2::GattCharacteristic::Permission permission,
    api::ble_v2::GattCharacteristic::Property property)
  override;

  bool UpdateCharacteristic(
    const api::ble_v2::GattCharacteristic& characteristic, const nearby::ByteArray& value)
  override;

  absl::Status NotifyCharacteristicChanged(
    const api::ble_v2::GattCharacteristic& characteristic, bool confirm,
    const ByteArray& new_value)
  override;

  void Stop() override;
};

class BleV2Socket: public api::ble_v2::BleSocket
{
  InputStream& GetInputStream() override;
  OutputStream& GetOutputStream() override;
  Exception Close() override;
};

class BleV2ServerSocket: public api::ble_v2::BleServerSocket
{
  std::unique_ptr<api::ble_v2::BleSocket> Accept() override;
  Exception Close() override;
};
}
}
