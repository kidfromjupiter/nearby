#pragma once

#include "ftxui/dom/elements.hpp"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

enum class ShareTargetType {
  // Unknown device type.
  kUnknown = 0,
  // A phone.
  kPhone = 1,
  // A tablet.
  kTablet = 2,
  // A laptop.
  kLaptop = 3,
  // A car.
  kCar = 4,
  // A foldable.
  kFoldable = 5,
  // An XR device.
  kXR = 6,
};

Element ShareTarget(
      std::string device_name, 
      ShareTargetType device_type
    );

}  // namespace nearby::sharing::linux_tui
