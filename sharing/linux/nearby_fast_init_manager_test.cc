#include "sharing/linux/nearby_fast_init_manager.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "absl/synchronization/mutex.h"
#include "gtest/gtest.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/implementation/ble.h"
#include "internal/platform/uuid.h"
#include "sharing/linux/nearby_fast_init_ble_beacon.h"

namespace nearby::sharing::linux {

class LinuxFastInitiationManagerTestPeer {
 public:
  static void BeginScan(LinuxFastInitiationManager& manager,
                        std::function<void()> discovered,
                        std::function<void()> not_discovered) {
    absl::MutexLock lock(manager.mutex_);
    manager.is_scanning_ = true;
    manager.scan_generation_ = 1;
    manager.devices_discovered_callback_ = std::move(discovered);
    manager.devices_not_discovered_callback_ = std::move(not_discovered);
    manager.discovered_peripherals_.clear();
  }

  static void Found(LinuxFastInitiationManager& manager, uint64_t id,
                    api::ble::BleAdvertisementData data) {
    manager.OnAdvertisementFound(1, id, std::move(data));
  }

  static void Lost(LinuxFastInitiationManager& manager, uint64_t id) {
    manager.OnAdvertisementLost(1, id);
  }

  static bool IsValid(LinuxFastInitiationManager& manager,
                      const api::ble::BleAdvertisementData& data) {
    return manager.IsFastInitAdvertisement(data);
  }
};

namespace {

api::ble::BleAdvertisementData FastInitAdvertisement(
    api::FastInitBleBeacon::FastInitType type =
        api::FastInitBleBeacon::FastInitType::kNotify) {
  std::string bytes(api::FastInitBleBeacon::kAdvertiseDataTotalSize -
                        api::FastInitBleBeacon::kFastInitServiceUuidSize,
                    '\0');
  for (size_t i = 0; i < api::FastInitBleBeacon::kFastInitModelIdSize; ++i) {
    bytes[i] = static_cast<char>(api::FastInitBleBeacon::kFastInitModelId[i]);
  }
  bytes[api::FastInitBleBeacon::kFastInitModelIdSize] =
      static_cast<char>(static_cast<uint8_t>(type) << 2);

  api::ble::BleAdvertisementData data;
  data.service_data.emplace(*Uuid::FromString(kFastInitServiceUuid),
                            ByteArray(bytes));
  return data;
}

TEST(LinuxFastInitiationManagerTest, ValidatesFastInitServiceData) {
  LinuxFastInitBleBeacon beacon;
  LinuxFastInitiationManager manager(beacon, nullptr);

  EXPECT_TRUE(LinuxFastInitiationManagerTestPeer::IsValid(
      manager, FastInitAdvertisement()));
  EXPECT_TRUE(LinuxFastInitiationManagerTestPeer::IsValid(
      manager,
      FastInitAdvertisement(api::FastInitBleBeacon::FastInitType::kSilent)));

  auto malformed = FastInitAdvertisement();
  malformed.service_data.begin()->second.data()[0] = '\0';
  EXPECT_FALSE(LinuxFastInitiationManagerTestPeer::IsValid(manager, malformed));
}

TEST(LinuxFastInitiationManagerTest, ReportsOnlyPresenceTransitions) {
  LinuxFastInitBleBeacon beacon;
  LinuxFastInitiationManager manager(beacon, nullptr);
  int discovered_count = 0;
  int not_discovered_count = 0;
  LinuxFastInitiationManagerTestPeer::BeginScan(
      manager, [&] { ++discovered_count; }, [&] { ++not_discovered_count; });

  LinuxFastInitiationManagerTestPeer::Found(manager, 1,
                                            FastInitAdvertisement());
  LinuxFastInitiationManagerTestPeer::Found(manager, 1,
                                            FastInitAdvertisement());
  LinuxFastInitiationManagerTestPeer::Found(manager, 2,
                                            FastInitAdvertisement());
  EXPECT_EQ(discovered_count, 1);

  LinuxFastInitiationManagerTestPeer::Lost(manager, 99);
  LinuxFastInitiationManagerTestPeer::Lost(manager, 1);
  EXPECT_EQ(not_discovered_count, 0);
  LinuxFastInitiationManagerTestPeer::Lost(manager, 2);
  EXPECT_EQ(not_discovered_count, 1);
}

TEST(LinuxFastInitiationManagerTest, StopScanningClearsLogicalState) {
  LinuxFastInitBleBeacon beacon;
  LinuxFastInitiationManager manager(beacon, nullptr);
  LinuxFastInitiationManagerTestPeer::BeginScan(manager, [] {}, [] {});
  bool stopped = false;

  manager.StopScanning([&] { stopped = true; });

  EXPECT_TRUE(stopped);
  EXPECT_FALSE(manager.IsScanning());
}

}  // namespace
}  // namespace nearby::sharing::linux
