#include "internal/platform/implementation/linux/bluetooth_adapter.h"


#include <gtest/gtest.h>
namespace nearby {
namespace linux {


class MockBluetoothAdapter: public BluetoothAdapter {

};

class BleV2MediumTests: public ::testing::Test {
public:
  static void SetupTestSuite() {

  }
  static void TearDownTestSuite() {

  }
};
}
}