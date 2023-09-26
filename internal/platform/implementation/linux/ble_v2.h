#include "bluetooth_adapter.h"
#include "internal/platform/implementation/ble_v2.h"
#include "internal/platform/implementation/linux/stream.h"

namespace nearby {
namespace linux {
class BleV2Peripheral : public api::ble_v2::BlePeripheral
{
  std::string GetAddress() const override;
  UniqueId GetUniqueId() const override;
};

class BleV2Socket : public api::ble_v2::BleSocket {
 public:
  BleV2Socket() = delete;
  explicit BleV2Socket(sdbus::UnixFd fd): input_stream_(fd), output_stream_(fd) {};
  ~BleV2Socket() override = default;

  nearby::InputStream& GetInputStream() override { return input_stream_;};

  nearby::OutputStream& GetOutputStream() override {return output_stream_;};

  Exception Close() override
  {
    input_stream_.Close();
    output_stream_.Close();
    return Exception{Exception::kSuccess};
  };

  api::ble_v2::BlePeripheral* GetRemotePeripheral() override
  {
    return ble_peripheral_;
  };


 private:
  linux::InputStream input_stream_;
  linux::OutputStream output_stream_;
  api::ble_v2::BlePeripheral* ble_peripheral_ = nullptr;
};

class GattClient : public api::ble_v2::GattClient
{
  bool DiscoverServiceAndCharacteristics(const Uuid &service_uuid,
                                         const std::vector<Uuid> &characteristic_uuids) override;

  absl::optional<api::ble_v2::GattCharacteristic> GetCharacteristic(const Uuid &service_uuid,
                                                                    const Uuid &characteristic_uuid) override;

  absl::optional<std::string> ReadCharacteristic(const api::ble_v2::GattCharacteristic &characteristic) override;

  bool WriteCharacteristic(const api::ble_v2::GattCharacteristic &characteristic, absl::string_view value,
                           WriteType type) override;

  bool SetCharacteristicSubscription(
    const api::ble_v2::GattCharacteristic &characteristic, bool enable,
    absl::AnyInvocable<void(absl::string_view value)> on_characteristic_changed_cb) override;

  void Disconnect() override;
};

class GattServer : public api::ble_v2::GattServer
{
  api::ble_v2::BlePeripheral &GetBlePeripheral() override;

  absl::optional<api::ble_v2::GattCharacteristic>
  CreateCharacteristic(const Uuid &service_uuid, const Uuid &characteristic_uuid,
                       api::ble_v2::GattCharacteristic::Permission permission,
                       api::ble_v2::GattCharacteristic::Property property) override;

  bool UpdateCharacteristic(const api::ble_v2::GattCharacteristic &characteristic,
                            const nearby::ByteArray &value) override;

  absl::Status NotifyCharacteristicChanged(const api::ble_v2::GattCharacteristic &characteristic, bool confirm,
                                           const ByteArray &new_value) override;

  void Stop() override;
};

class BleV2ServerSocket : public api::ble_v2::BleServerSocket
{
public:
  explicit BleV2ServerSocket(api::BluetoothAdapter& adapter);
  ~BleV2ServerSocket() override = default;
  std::unique_ptr<api::ble_v2::BleSocket> Accept() override;
  Exception Close() override;
private:
  // Accept notification.
  absl::Mutex mutex_;
  absl::CondVar cond_;
  bool closed_ = false;
  linux::BluetoothAdapter *adapter_;
  std::deque<linux::BleV2Socket> pending_sockets_ ABSL_GUARDED_BY(mutex_);
};

class BleV2Medium : public api::ble_v2::BleMedium
{
public:
  BleV2Medium(const BleV2Medium &) = delete;
  BleV2Medium(BleV2Medium &&) = delete;
  BleV2Medium &operator=(const BleV2Medium &) = delete;
  BleV2Medium &operator=(BleV2Medium &&) = delete;

  explicit BleV2Medium(api::BluetoothAdapter &adapter);
  ~BleV2Medium() override = default;

  bool StartAdvertising(const api::ble_v2::BleAdvertisementData &advertising_data,
                        api::ble_v2::AdvertiseParameters advertise_set_parameters) override;

  std::unique_ptr<AdvertisingSession> StartAdvertising(const api::ble_v2::BleAdvertisementData &advertising_data,
                                                       api::ble_v2::AdvertiseParameters advertise_set_parameters,
                                                       AdvertisingCallback callback) override;


  bool StopAdvertising() override;

  std::unique_ptr<ScanningSession> StartScanning(const Uuid &service_uuid, api::ble_v2::TxPowerLevel tx_power_level,
                                                 ScanningCallback callback) override;

  bool StartScanning(const Uuid &service_uuid, api::ble_v2::TxPowerLevel tx_power_level,
                     ScanCallback callback) override;

  bool StopScanning() override;

  std::unique_ptr<api::ble_v2::GattServer> StartGattServer(api::ble_v2::ServerGattConnectionCallback callback) override;

  std::unique_ptr<api::ble_v2::GattClient>
  ConnectToGattServer(api::ble_v2::BlePeripheral &peripheral, api::ble_v2::TxPowerLevel tx_power_level,
                      api::ble_v2::ClientGattConnectionCallback callback) override;

  std::unique_ptr<api::ble_v2::BleServerSocket> OpenServerSocket(const std::string &service_id) override;

  std::unique_ptr<api::ble_v2::BleSocket> Connect(const std::string &service_id,
                                                  api::ble_v2::TxPowerLevel tx_power_level,
                                                  api::ble_v2::BlePeripheral &peripheral,
                                                  CancellationFlag *cancellation_flag) override;


  bool IsExtendedAdvertisementsAvailable() override;
  bool GetRemotePeripheral(const std::string &mac_address, GetRemotePeripheralCallback callback) override;
  bool GetRemotePeripheral(api::ble_v2::BlePeripheral::UniqueId id, GetRemotePeripheralCallback callback) override;

private:
  linux::BluetoothAdapter *adapter_;
  std::unique_ptr<bluez::BluezObjectManager> bluez_object_manager_;
};
} // namespace linux
} // namespace nearby
