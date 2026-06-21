#include "sharing/linux/tui/components/tablet_icon.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element Tablet() {
  return vbox({
      text("____________") | center,
      text("|          |") | center,
      text("|          |") | center,
      text("|__________|") | center,
      text("            ") | center,
  });
}
}  // namespace nearby::sharing::linux_tui
