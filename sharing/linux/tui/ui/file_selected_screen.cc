#include "sharing/linux/tui/ui/file_selected_screen.h"

#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element FileSelectedScreen(const std::string& selected_file) {
  return vbox({
             text("File selected") | bold | center | color(Palette::primary),
             separator(),
             paragraph(selected_file) | center,
             separatorEmpty(),
             text("Press Backspace to choose another file") | dim | center,
         }) |
         borderStyled(Palette::primary) | flex;
}

}  // namespace nearby::sharing::linux_tui
