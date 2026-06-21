#include "sharing/linux/tui/ui/home_screen.h"

#include "ftxui/dom/elements.hpp"
#include "sharing/linux/tui/components/file_picker_card.h"
#include "sharing/linux/tui/components/incoming_share_card.h"
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
  auto incoming_share_card =
      IncomingShareCard({.device_name = options.incoming_share_device_name,
                         .device_type = options.incoming_share_device_type,
                         .on_accept = options.on_incoming_share_accept,
                         .on_decline = options.on_incoming_share_decline});
  auto content = Container::Vertical({file_picker_card, incoming_share_card});

  return Renderer(content, [file_picker_card, incoming_share_card, options] {
    const std::string selected_file =
        options.selected_file == nullptr ? "" : *options.selected_file;
    const Page current_page = options.current_page == nullptr
                                  ? Page::FilePicker
                                  : *options.current_page;

    Element main_panel;
    switch (current_page) {
      case Page::FilePicker:
        main_panel = file_picker_card->Render();
        break;
      case Page::IncomingShare:
        main_panel = incoming_share_card->Render();
        break;
      case Page::Sharing:
        main_panel = FileSelectedScreen(selected_file);
        break;
    }

    return vbox({
               HomeHeader() | color(Palette::accent),
               separator(),
               hbox({
                   Sidebar({.hostname = options.hostname,
                            .selected_file = selected_file}),
                   separator() | color(Palette::border),
                   main_panel | flex,
               }) | flex,
           }) |
           bgcolor(Palette::base) | size(WIDTH, GREATER_THAN, 80) |
           size(HEIGHT, GREATER_THAN, 24);
  });
}

}  // namespace nearby::sharing::linux_tui
