#include "sharing/linux/tui/components/file_picker_card.h"

#include <memory>

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/screen/box.hpp"
#include "sharing/linux/tui/components/file_icon.h"
#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {

ftxui::Component FilePickerCard(FilePickerCardOptions options) {
  auto picker_box = std::make_shared<ftxui::Box>();
  auto hovered = std::make_shared<bool>(false);

  auto card = ftxui::Renderer([picker_box, hovered] {
    auto picker_card =
        ftxui::vbox({
            FileIcon(FileIconSize::Large) | ftxui::color(Palette::secondary),
            ftxui::separatorEmpty(),
            ftxui::text("Select file to share") | ftxui::bold | ftxui::center,
        }) |
        ftxui::borderStyled(*hovered ? Palette::primary : Palette::secondary) |
        ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 33) |
        ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 13) |
        ftxui::reflect(*picker_box);

    return ftxui::vbox({
               ftxui::filler(),
               ftxui::hbox({
                   ftxui::filler(),
                   picker_card,
                   ftxui::filler(),
               }),
               ftxui::filler(),
           }) |
           ftxui::flex;
  });

  return ftxui::CatchEvent(
      card, [picker_box, hovered, options](ftxui::Event event) {
        if (!event.is_mouse()) {
          return false;
        }

        auto mouse = event.mouse();
        bool inside_picker =
            mouse.x >= picker_box->x_min && mouse.x <= picker_box->x_max &&
            mouse.y >= picker_box->y_min && mouse.y <= picker_box->y_max;

        *hovered = inside_picker;

        if (!inside_picker || mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed ||
            options.file_picker == nullptr) {
          return false;
        }

        std::string path = options.file_picker->PickFile();
        if (!path.empty() && options.on_file_selected) {
          options.on_file_selected(path);
        }

        return true;
      });
}

}  // namespace nearby::sharing::linux_tui
