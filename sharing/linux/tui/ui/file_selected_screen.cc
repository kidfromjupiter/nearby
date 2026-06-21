#include "sharing/linux/tui/ui/file_selected_screen.h"

#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {

ftxui::Element FileSelectedScreen(const std::string& selected_file) {
  return ftxui::vbox({
             ftxui::text("File selected") | ftxui::bold | ftxui::center |
                 ftxui::color(Palette::primary),
             ftxui::separator(),
             ftxui::paragraph(selected_file) | ftxui::center,
             ftxui::separatorEmpty(),
             ftxui::text("Press Backspace to choose another file") |
                 ftxui::dim | ftxui::center,
         }) |
         ftxui::borderStyled(Palette::primary) | ftxui::flex;
}

}  // namespace nearby::sharing::linux_tui
