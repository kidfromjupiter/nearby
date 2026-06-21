#include "sharing/linux/tui/components/file_icon.h"

namespace nearby::sharing::linux_tui {

ftxui::Element FileIcon(FileIconSize size) {
  if (size == FileIconSize::Small) {
    return ftxui::vbox({
        ftxui::text("    ______ ") | ftxui::center,
        ftxui::text("   / |   | ") | ftxui::center,
        ftxui::text("  /__|   | ") | ftxui::center,
        ftxui::text(" | ----- | ") | ftxui::center,
        ftxui::text(" | ----- | ") | ftxui::center,
        ftxui::text(" |_______| ") | ftxui::center,
    });
  }

  return ftxui::vbox({
      ftxui::text("    __________ ") | ftxui::center,
      ftxui::text("   / |       | ") | ftxui::center,
      ftxui::text("  /__|       | ") | ftxui::center,
      ftxui::text(" | --------- | ") | ftxui::center,
      ftxui::text(" | --------- | ") | ftxui::center,
      ftxui::text(" | --------- | ") | ftxui::center,
      ftxui::text(" | --------- | ") | ftxui::center,
      ftxui::text(" |___________| ") | ftxui::center,
  });
}

}  // namespace nearby::sharing::linux_tui
