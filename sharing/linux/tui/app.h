#pragma once

#include "ftxui/component/screen_interactive.hpp"
#include "sharing/linux/tui/file_picker.h"
#include "sharing/linux/tui/page.h"

#include <string>

namespace nearby::sharing::linux_tui {

class TuiApp {
 public:
  TuiApp();

  int Run();

 private:
  bool HandleEvent(ftxui::Event event);

  ftxui::ScreenInteractive screen_;
  ZenityFilePicker file_picker_;
  Page current_page_ = Page::FilePicker;
  std::string hostname_;
  std::string selected_file_;
};

}  // namespace nearby::sharing::linux_tui
