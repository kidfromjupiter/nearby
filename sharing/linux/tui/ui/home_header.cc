#include "sharing/linux/tui/ui/home_header.h"
#include "sharing/linux/tui/components/brand.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element HomeHeader() {
  return Brand();
}

}  // namespace nearby::sharing::linux_tui
