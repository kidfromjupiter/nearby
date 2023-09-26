#include <vector>

#include "internal/platform/implementation/ble_v2.h"
#include "internal/platform/implementation/linux/bluez.h"
#include "internal/platform/implementation/linux/ble_v2_advertisement.h"
#include "internal/platform/logging.h"
#include "internal/platform/uuid.h"

namespace nearby {
namespace linux {
LEAdvertisement::LEAdvertisement(
    sdbus::IConnection& system_bus, sdbus::ObjectPath path,
    const api::ble_v2::BleAdvertisementData& advertising_data,
    api::ble_v2::AdvertiseParameters advertise_set_parameters)
    : AdaptorInterfaces(system_bus, std::move(path)),
      is_extended_advertisement_(advertising_data.is_extended_advertisement),
      advertise_set_parameters_(advertise_set_parameters) {
  for (const auto& [uuid, data] : advertising_data.service_data) {
    std::string uuid_string(uuid);
    std::vector<uint8_t> data_bytes(data.size());
    const auto* bytes = data.data();

    service_uuids_.push_back(uuid_string);
    for (size_t i = 0; i < data.size(); i++) {
      data_bytes[i] = bytes[i];
    }

    service_data_.insert({uuid_string, std::move(data_bytes)});
  }

  registerAdaptor();

  NEARBY_VLOG(1) << __func__
                       << ": Created a org.bluez.LEAdvertisement1 instance at "
                       << getObjectPath();
}
}  // namespace linux
}  // namespace nearby
