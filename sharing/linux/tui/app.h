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

  ScreenInteractive screen_;
  ZenityFilePicker file_picker_;
  std::string hostname_;
  std::string selected_file_;
};

}  // namespace nearby::sharing::linux_tui
