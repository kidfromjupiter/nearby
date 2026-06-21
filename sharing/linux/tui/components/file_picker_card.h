#pragma once

#include <functional>
#include <string>

#include "ftxui/component/component.hpp"
#include "sharing/linux/tui/file_picker.h"

namespace nearby::sharing::linux_tui {

struct FilePickerCardOptions {
  FilePicker* file_picker = nullptr;
  std::function<void(std::string)> on_file_selected;
};

ftxui::Component FilePickerCard(FilePickerCardOptions options);

}  // namespace nearby::sharing::linux_tui
