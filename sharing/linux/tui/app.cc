#include "sharing/linux/tui/app.h"

#include <unistd.h>

#include <string>

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
      .incoming_share_device_name = incoming_share_device_name_,
      .incoming_share_device_type = incoming_share_device_type_,
      .on_incoming_share_accept =
          [this]() { current_page_ = Page::FilePicker; },
      .on_incoming_share_decline =
          [this]() { current_page_ = Page::FilePicker; },
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

  if (event == Event::Backspace) {
    if (current_page_ == Page::Sharing) {
      current_page_ = Page::FilePicker;
      selected_file_.clear();
      return true;
    }
    if (current_page_ == Page::IncomingShare) {
      current_page_ = Page::FilePicker;
      return true;
    }
  }

  return false;
}

}  // namespace nearby::sharing::linux_tui
