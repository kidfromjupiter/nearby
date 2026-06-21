#pragma once

#include "ftxui/dom/elements.hpp"

namespace nearby::sharing::linux_tui {

enum class FileIconSize {
  Large,
  Small,
};

ftxui::Element FileIcon(FileIconSize size);

}  // namespace nearby::sharing::linux_tui
