// Copyright 2020 Google LLC
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

#include "connections/implementation/injected_bluetooth_device_store.h"

#include <cstdint>
#include <string>

#include "absl/status/statusor.h"
#include "connections/implementation/bluetooth_device_name.h"
#include "internal/platform/implementation/bluetooth_classic.h"
#include "internal/platform/mac_address.h"

namespace nearby {
namespace connections {
namespace {

// api::BluetoothDevice implementation which stores a name and address passed to
// its constructor and trivially returns them to implement virtual functions.
class InjectedBluetoothDevice : public api::BluetoothDevice {
 public:
  InjectedBluetoothDevice(const std::string& name,
                          const std::string& mac_address)
      : name_(name), mac_address_(mac_address) {}

  ~InjectedBluetoothDevice() override = default;

  // api::BluetoothDevice:
  std::string GetName() const override { return name_; }

  std::string GetMacAddress() const override { return mac_address_; }

 private:
  const std::string name_;
  const std::string mac_address_;
};

}  // namespace

InjectedBluetoothDeviceStore::InjectedBluetoothDeviceStore() = default;

InjectedBluetoothDeviceStore::~InjectedBluetoothDeviceStore() = default;

BluetoothDevice InjectedBluetoothDeviceStore::CreateInjectedBluetoothDevice(
    const ByteArray& remote_bluetooth_mac_address,
    const std::string& endpoint_id, const ByteArray& endpoint_info,
    const ByteArray& service_id_hash, Pcp pcp) {
  // Valid MAC address is required.
  absl::StatusOr<uint64_t> bluetooth_mac_address_bytes_uint64 =
      remote_bluetooth_mac_address.Read6BytesAsUint64();
  if (!bluetooth_mac_address_bytes_uint64.ok()) {
    return BluetoothDevice(/*device=*/nullptr);
  }

  MacAddress remote_mac_address;
  if (!MacAddress::FromUint64(bluetooth_mac_address_bytes_uint64.value(),
                              remote_mac_address)
      || !remote_mac_address.IsSet()) {
    return BluetoothDevice(/*device=*/nullptr);
  }

  // Non-empty endpoint info is required.
  if (endpoint_info.Empty()) return BluetoothDevice(/*device=*/nullptr);

  BluetoothDeviceName name(BluetoothDeviceName::Version::kV1, pcp, endpoint_id,
                           service_id_hash, endpoint_info,
                           /*uwb_address=*/ByteArray(),
                           WebRtcState::kConnectable);

  // Note: BluetoothDeviceName internally verifies that |endpoint_id| and
  // |service_id_hash| are valid; the check below will fail if they are
  // malformed.
  if (!name.IsValid()) return BluetoothDevice(/*device=*/nullptr);

  auto injected_device = std::make_unique<InjectedBluetoothDevice>(
      static_cast<std::string>(name), remote_mac_address.ToString());
  BluetoothDevice device_to_return(injected_device.get());

  // Store underlying device to ensure that it is kept alive for future use.
  devices_.emplace_back(std::move(injected_device));

  return device_to_return;
}

}  // namespace connections
}  // namespace nearby
