#include <utility>

#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <systemd/sd-bus.h>

#include "internal/platform/implementation/bluetooth_classic.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"
#include "internal/platform/implementation/linux/bluetooth_pairing.h"
#include "internal/platform/logging.h"

namespace nearby {
namespace linux {

void BluetoothPairing::pairing_reply_handler(const sdbus::Error *error) {
  if (error != nullptr && error->isValid()) {
    auto name = error->getName();
    api::BluetoothPairingCallback::PairingError err;

    NEARBY_LOGS(ERROR) << __func__ << ": "
                       << "Got error '" << error->getName()
                       << "' with message '" << error->getMessage()
                       << "' while pairing with device "
                       << device_.getObjectPath();

    if (name == "org.bluez.Error.AuthenticationCanceled") {
      err = api::BluetoothPairingCallback::PairingError::kAuthCanceled;
    } else if (name == "org.bluez.Error.AuthenticationFailed") {
      err = api::BluetoothPairingCallback::PairingError::kAuthFailed;
    } else if (name == "org.bluez.Error.AuthenticationRejected") {
      err = api::BluetoothPairingCallback::PairingError::kAuthRejected;
    } else if (name == "org.bluez.Error.AuthenticationTimeout") {
      err = api::BluetoothPairingCallback::PairingError::kAuthTimeout;
    } else {
      err = api::BluetoothPairingCallback::PairingError::kAuthFailed;
    }

    if (pairing_cb_.on_pairing_error_cb != nullptr) {
      pairing_cb_.on_pairing_error_cb(err);
    }

    return;
  }
  if (pairing_cb_.on_paired_cb != nullptr) {
    pairing_cb_.on_paired_cb();
  }
  return;
}

BluetoothPairing::BluetoothPairing(BluetoothAdapter &adapter,
				   BluetoothDevice &remote_device)
    : device_(remote_device), adapter_(adapter) {}

bool BluetoothPairing::InitiatePairing(
    api::BluetoothPairingCallback pairing_cb) {
  pairing_cb_ = std::move(pairing_cb);
  if (pairing_cb_.on_pairing_initiated_cb != nullptr)
    pairing_cb_.on_pairing_initiated_cb(api::PairingParams{
        api::PairingParams::PairingType::kConsent, std::string()});

  return true;
}

bool BluetoothPairing::FinishPairing(
    std::optional<absl::string_view> pin_code) {
  device_.set_pair_reply_callback([this](const sdbus::Error *error) {
    this->pairing_reply_handler(error);
  });

  try {
    pair_async_call_ = device_.Pair();
  } catch (const sdbus::Error &e) {
    NEARBY_LOGS(ERROR) << __func__ << ": Got error '" << e.getName()
                       << "' with message '" << e.getMessage()
                       << "' while trying to initiate pairing for device "
                       << device_.getObjectPath();
    return false;
  }

  return true;
}

bool BluetoothPairing::CancelPairing() {
  try {
    if (pair_async_call_.isPending()) {
      pair_async_call_.cancel();
    }

    device_.CancelPairing();
  } catch (const sdbus::Error &e) {
    NEARBY_LOGS(ERROR) << __func__ << ": Got error '" << e.getName()
                       << "' with message '" << e.getMessage()
                       << "' while trying to cancel pairing for device "
                       << device_.getObjectPath();
    return false;
  }

  return true;
}

bool BluetoothPairing::Unpair() {
  try {
    adapter_.RemoveDevice(device_.getObjectPath());
    return true;
  } catch (const sdbus::Error &e) {
    NEARBY_LOGS(ERROR) << __func__ << ": Got error '" << e.getName()
                       << "' with message '" << e.getMessage()
                       << "' while trying to unpair device "
                       << device_.getObjectPath() << " on adapter "
                       << adapter_.getObjectPath();
    return false;
  }
}

bool BluetoothPairing::IsPaired() {
  try {
    bool bonded = device_.Bonded();
    return bonded;
  } catch (const sdbus::Error &e) {
    NEARBY_LOGS(ERROR) << __func__ << ": Got error '" << e.getName()
                       << "' with message '" << e.getMessage()
                       << "' while trying to get Bonded state for device "
                       << device_.getObjectPath();
    return false;
  }
}
} // namespace linux
} // namespace nearby
