#include "sharing/linux/tui/components/laptop_icon.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element Laptop() {
  return vbox({
      text(" +========+ ") | center,
      text(" |        | ") | center,
      text(" |________| ") | center,
      text(" /--------\\ ") | center,
      text("/__________\\") | center,
  });
}
}  // namespace nearby::sharing::linux_tui
