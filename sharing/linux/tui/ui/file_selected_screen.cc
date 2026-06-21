#include "sharing/linux/tui/ui/file_selected_screen.h"
#include "sharing/linux/tui/components/share_target.h"

#include "sharing/linux/tui/palette.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

Element FileSelectedScreen(const std::string& selected_file) {
  FlexboxConfig config;
  config.direction = FlexboxConfig::Direction::Row;
  config.wrap = FlexboxConfig::Wrap::Wrap;
  config.gap_x = 2;
  config.gap_y = 1;
  return vbox({
            flexbox({
                  ShareTarget("Lasan's A55", ShareTargetType::kPhone),
                  ShareTarget("lasan-laptop", ShareTargetType::kLaptop),
                  ShareTarget("lasan-laptop", ShareTargetType::kLaptop),
                  ShareTarget("lasan-laptop", ShareTargetType::kLaptop),
                  ShareTarget("Lasan's S9+", ShareTargetType::kTablet),
                  ShareTarget("Lasan's S9+", ShareTargetType::kTablet),

                }, config) | flex
         }) |
         borderStyled(Palette::primary) | flex;
}

}  // namespace nearby::sharing::linux_tui
