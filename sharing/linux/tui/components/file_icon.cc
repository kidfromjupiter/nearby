#include "sharing/linux/tui/components/file_icon.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element FileIcon(FileIconSize size) {
  if (size == FileIconSize::Small) {
    return vbox({
        text("    ______ ") | center,
        text("   / |   | ") | center,
        text("  /__|   | ") | center,
        text(" | ----- | ") | center,
        text(" | ----- | ") | center,
        text(" |_______| ") | center,
    });
  }

  return vbox({
      text("    __________ ") | center,
      text("   / |       | ") | center,
      text("  /__|       | ") | center,
      text(" | --------- | ") | center,
      text(" | --------- | ") | center,
      text(" | --------- | ") | center,
      text(" | --------- | ") | center,
      text(" |___________| ") | center,
  });
}

}  // namespace nearby::sharing::linux_tui
