#include "sharing/linux/tui/app.h"

#include <unistd.h>

#include "ftxui/component/component.hpp"
#include "sharing/linux/tui/ui/home_screen.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;
namespace {

std::string GetHostname() {
  char hostname[256] = {};
  gethostname(hostname, sizeof(hostname));
  return hostname;
}

}  // namespace

TuiApp::TuiApp() : screen_(ScreenInteractive::Fullscreen()) {
  hostname_ = GetHostname();
  screen_.TrackMouse(true);
}

int TuiApp::Run() {
  auto component = HomeScreen({
      .hostname = hostname_,
      .current_page = &current_page_,
      .selected_file = &selected_file_,
      .file_picker = &file_picker_,
      .on_file_selected =
          [this](std::string path) {
            selected_file_ = path;
            current_page_ = Page::Sharing;
          },
  });

  auto app =
      CatchEvent(component, [this](Event event) { return HandleEvent(event); });

  screen_.Loop(app);
  return 0;
}

bool TuiApp::HandleEvent(Event event) {
  if (event == Event::Character('q') || event == Event::Escape) {
    screen_.ExitLoopClosure()();
    return true;
  }

  if (event == Event::Backspace && current_page_ == Page::Sharing) {
    current_page_ = Page::FilePicker;
    selected_file_.clear();
    return true;
  }

  return false;
}

}  // namespace nearby::sharing::linux_tui
