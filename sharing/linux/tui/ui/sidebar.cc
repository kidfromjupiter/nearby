#include "sharing/linux/tui/ui/sidebar.h"

#include "sharing/linux/tui/components/file_icon.h"
#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element Sidebar(const SidebarState& state) {
  auto visible_as_box =
      window(
          text(" Visible as "),
          vbox({
              text(""),
              text(state.hostname) | bold | center,
              text(""),
          })) |
      size(WIDTH, GREATER_THAN, 24) |
      size(HEIGHT, EQUAL, 5);

  if (state.selected_file.empty()) {
    return vbox({
        visible_as_box,
        filler(),
    });
  }

  return vbox({
      visible_as_box,
      filler(),
      window(
          text(" Selected to share ") | center,
          vbox({
              FileIcon(FileIconSize::Small) | color(Palette::accent),
              text(""),
              paragraph(state.selected_file) | dim,
          })) |
          size(WIDTH, EQUAL, 24),
  });
}

}  // namespace nearby::sharing::linux_tui
