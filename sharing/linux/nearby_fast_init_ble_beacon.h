#include "sharing/internal/api/fast_init_ble_beacon.h"

namespace nearby {
namespace sharing {
namespace linux {

class LinuxFastInitBleBeacon final : public nearby::api::FastInitBleBeacon {
 public:
  void SerializeToByteArray() override {
    std::array<uint8_t, nearby::api::FastInitBleBeacon::kAdvertiseDataTotalSize>
        data{};
    std::size_t offset = 0;

    data[offset++] = nearby::api::FastInitBleBeacon::kFastInitServiceUuid[0];
    data[offset++] = nearby::api::FastInitBleBeacon::kFastInitServiceUuid[1];

    for (uint8_t byte : nearby::api::FastInitBleBeacon::kFastInitModelId) {
      data[offset++] = byte;
    }

    const uint8_t metadata = (static_cast<uint8_t>(this->GetVersion()) << 5) |
                             (static_cast<uint8_t>(this->GetType()) << 2) |
                             (this->GetUwbSupported() ? 0x02 : 0x00) |
                             (this->GetSenderCertSupported() ? 0x01 : 0x00);
    data[offset++] = metadata;
    data[offset++] = static_cast<uint8_t>(this->GetAdjustedTxPower());

    for (uint8_t byte : this->GetUwbMetadata()) {
      data[offset++] = byte;
    }
    for (uint8_t byte : this->GetUwbAddress()) {
      data[offset++] = byte;
    }
    for (uint8_t byte : this->GetSalt()) {
      data[offset++] = byte;
    }
    for (uint8_t byte : this->GetSecretIdHash()) {
      data[offset++] = byte;
    }

    data[offset++] = (this->GetRequireBtAdvertising() ? 0x80 : 0x00) |
                     (this->GetSelfOnlyAdvertising() ? 0x40 : 0x00);
    SetAdDataByteArray(data);
  }

  void ParseFromByteArray() override {
    const auto data = GetAdDataByteArray();
    if (data[0] != nearby::api::FastInitBleBeacon::kFastInitServiceUuid[0] ||
        data[1] != nearby::api::FastInitBleBeacon::kFastInitServiceUuid[1]) {
      return;
    }

    const uint8_t metadata = data[5];
    SetVersion(static_cast<FastInitVersion>((metadata >> 5) & 0x07));
    SetType(static_cast<FastInitType>((metadata >> 2) & 0x07));
    SetUwbSupported((metadata & 0x02) != 0);
    SetSenderCertSupported((metadata & 0x01) != 0);
    SetAdjustedTxPower(static_cast<int8_t>(data[6]));

    std::array<uint8_t, kUwbMetadataSize> uwb_metadata{};
    uwb_metadata[0] = data[7];
    SetUwbMetadata(uwb_metadata);

    std::array<uint8_t, kUwbAddressSize> uwb_address{};
    for (std::size_t i = 0; i < uwb_address.size(); ++i) {
      uwb_address[i] = data[8 + i];
    }
    SetUwbAddress(uwb_address);

    std::array<uint8_t, kSaltSize> salt{};
    salt[0] = data[16];
    SetSalt(salt);

    std::array<uint8_t, kSecretIdHashSize> secret_id_hash{};
    for (std::size_t i = 0; i < secret_id_hash.size(); ++i) {
      secret_id_hash[i] = data[17 + i];
    }
    SetSecretIdHash(secret_id_hash);

    const uint8_t flags = data[25];
    SetRequireBtAdvertising((flags & 0x80) != 0);
    SetSelfOnlyAdvertising((flags & 0x40) != 0);
  }
};
}  // namespace linux
}  // namespace sharing
}  // namespace nearby
