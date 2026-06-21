#include "sharing/linux/tui/ui/home_screen.h"

#include "ftxui/dom/elements.hpp"
#include "sharing/linux/tui/components/file_picker_card.h"
#include "sharing/linux/tui/palette.h"
#include "sharing/linux/tui/ui/file_selected_screen.h"
#include "sharing/linux/tui/ui/home_header.h"
#include "sharing/linux/tui/ui/sidebar.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Component HomeScreen(HomeScreenOptions options) {
  auto file_picker_card =
      FilePickerCard({.file_picker = options.file_picker,
                      .on_file_selected = options.on_file_selected});

  return Renderer(file_picker_card, [file_picker_card, options] {
    const std::string selected_file =
        options.selected_file == nullptr ? "" : *options.selected_file;
    const Page current_page = options.current_page == nullptr
                                  ? Page::FilePicker
                                  : *options.current_page;

    auto main_panel = current_page == Page::FilePicker
                          ? file_picker_card->Render()
                          : FileSelectedScreen(selected_file);

    return vbox({
               HomeHeader() | color(Palette::accent),
               separator(),
               hbox({
                   Sidebar({.hostname = options.hostname,
                            .selected_file = selected_file}),
                   main_panel | flex,
               }) | flex,
           }) |
           bgcolor(Palette::base) | size(WIDTH, GREATER_THAN, 80) |
           size(HEIGHT, GREATER_THAN, 24);
  });
}

}  // namespace nearby::sharing::linux_tui
