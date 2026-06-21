#pragma once

#include <string>

#include "ftxui/component/screen_interactive.hpp"
#include "sharing/linux/tui/components/share_target.h"
#include "sharing/linux/tui/file_picker.h"
#include "sharing/linux/tui/page.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

class TuiApp {
 public:
  TuiApp();

  int Run();

 private:
  bool HandleEvent(Event event);

  ScreenInteractive screen_;
  ZenityFilePicker file_picker_;
  Page current_page_ = Page::FilePicker;
  std::string hostname_;
  std::string selected_file_;
  std::string incoming_share_device_name_ = "Lasan's A55";
  ShareTargetType incoming_share_device_type_ = ShareTargetType::kPhone;
};

}  // namespace nearby::sharing::linux_tui
