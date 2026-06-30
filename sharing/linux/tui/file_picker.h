#pragma once

#include <string>

namespace nearby::sharing::linux_tui {

class FilePicker {
 public:
  virtual ~FilePicker() = default;

  virtual std::string PickFile() = 0;
};

class ZenityFilePicker final : public FilePicker {
 public:
  std::string PickFile() override;
};

}  // namespace nearby::sharing::linux_tui
