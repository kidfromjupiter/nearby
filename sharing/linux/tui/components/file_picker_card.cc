#include "sharing/linux/tui/components/file_picker_card.h"

#include <memory>

#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/screen/box.hpp"
#include "sharing/linux/tui/components/file_icon.h"
#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Component FilePickerCard(FilePickerCardOptions options) {
  auto picker_box = std::make_shared<Box>();
  auto hovered = std::make_shared<bool>(false);

  auto card = Renderer([picker_box, hovered] {
    auto picker_card =
        vbox({
            FileIcon(FileIconSize::Large) | color(Palette::secondary),
            separatorEmpty(),
            text("Select file to share") | bold | center,
        }) |
        borderStyled(*hovered ? Palette::primary : Palette::secondary) |
        size(WIDTH, GREATER_THAN, 33) | size(HEIGHT, EQUAL, 13) |
        reflect(*picker_box);

    return vbox({
               filler(),
               hbox({
                   filler(),
                   picker_card,
                   filler(),
               }),
               filler(),
           }) |
           flex;
  });

  return CatchEvent(card, [picker_box, hovered, options](Event event) {
    if (!event.is_mouse()) {
      return false;
    }

    auto mouse = event.mouse();
    bool inside_picker =
        mouse.x >= picker_box->x_min && mouse.x <= picker_box->x_max &&
        mouse.y >= picker_box->y_min && mouse.y <= picker_box->y_max;

    *hovered = inside_picker;

    if (!inside_picker || mouse.button != Mouse::Left ||
        mouse.motion != Mouse::Pressed || options.file_picker == nullptr) {
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
