#include "sharing/linux/tui/components/phone_icon.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element Phone() {
  return vbox({
      text("______") | center,
      text("|    |") | center,
      text("|    |") | center,
      text("|    |") | center,
      text("|____|") | center,
  });
}
}  // namespace nearby::sharing::linux_tui
