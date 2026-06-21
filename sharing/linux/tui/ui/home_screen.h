#pragma once

#include <functional>
#include <string>

#include "ftxui/component/component.hpp"
#include "sharing/linux/tui/components/share_target.h"
#include "sharing/linux/tui/file_picker.h"
#include "sharing/linux/tui/page.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

struct HomeScreenOptions {
  std::string hostname;
  const Page* current_page = nullptr;
  const std::string* selected_file = nullptr;
  FilePicker* file_picker = nullptr;
  std::function<void(std::string)> on_file_selected;
  std::string incoming_share_device_name;
  ShareTargetType incoming_share_device_type = ShareTargetType::kUnknown;
  std::function<void()> on_incoming_share_accept;
  std::function<void()> on_incoming_share_decline;
};

Component HomeScreen(HomeScreenOptions options);

}  // namespace nearby::sharing::linux_tui
