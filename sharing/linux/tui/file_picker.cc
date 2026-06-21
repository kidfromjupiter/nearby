#include "sharing/linux/tui/file_picker.h"

#include <cstdio>
#include <string>

namespace nearby::sharing::linux_tui {

std::string ZenityFilePicker::PickFile() {
  FILE* pipe = popen("zenity --file-selection 2>/dev/null", "r");
  if (pipe == nullptr) {
    return {};
  }

  char buffer[4096];
  std::string result;

  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result = buffer;
  }

  pclose(pipe);

  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result;
}

}  // namespace nearby::sharing::linux_tui
