#include "sharing/linux/tui/components/incoming_share_card.h"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "sharing/linux/tui/components/share_target.h"
#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Component IncomingShareCard(IncomingShareCardOptions options) {
  auto accept_button = Button("Accept", [options] {
    if (options.on_accept) {
      options.on_accept();
    }
  });
  auto decline_button = Button("Decline", [options] {
    if (options.on_decline) {
      options.on_decline();
    }
  });
  auto controls = Container::Horizontal({accept_button, decline_button});

  return Renderer(controls, [accept_button, decline_button, options] {
    return vbox({
               window(text(" Incoming share ") | center,
                      vbox({
                          paragraph("Do you want to accept this share?") |
                              bold | center,
                          separator(),
                          ShareTarget(options.device_name, options.device_type),
                          separator(),
                          hbox({
                              accept_button->Render() | flex,
                              separator(),
                              decline_button->Render() | flex,
                          }),
                      })),
           }) |
           borderStyled(Palette::border) | bgcolor(Palette::surface) |
           size(WIDTH, GREATER_THAN, 40);
  });
}

}  // namespace nearby::sharing::linux_tui
