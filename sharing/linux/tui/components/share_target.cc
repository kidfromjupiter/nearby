#include "ftxui/dom/elements.hpp"
#include "sharing/linux/tui/components/share_target.h"
#include "sharing/linux/tui/components/laptop_icon.h"
#include "sharing/linux/tui/components/phone_icon.h"
#include "sharing/linux/tui/components/tablet_icon.h"
#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element ShareTarget(std::string device_name, ShareTargetType device_type) {
  Element icon;
  switch (device_type) {
    case ShareTargetType::kPhone:
      icon = Phone();
      break;
    case ShareTargetType::kTablet:
      icon = Tablet();
      break;
    default:
      icon = Laptop();
      break;
  }
  return vbox(
             {icon, separatorLight() | dim,
              paragraph(device_name) | color(Palette::secondary) | bold | center

             }) |
         size(WIDTH, EQUAL, 14) | border;
};

}  // namespace nearby::sharing::linux_tui
