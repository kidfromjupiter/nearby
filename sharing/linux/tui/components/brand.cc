#include "sharing/linux/tui/components/brand.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element Brand() {
  return vbox({
      text("   ___       _    _     ___ _                 "),
      text("  / _ \\ _  _(_)__| |__ / __| |_  __ _ _ _ ___ "),
      text(" | (_) | || | / _| / / \\__ \\ ' \\/ _` | '_/ -_)"),
      text("  \\__\\_\\\\_,_|_\\__|_\\_\\ |___/_||_\\__,_|_| \\___|"),
      text("                                              "),
  });
}
}  // namespace nearby::sharing::linux_tui
