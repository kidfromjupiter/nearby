#pragma once

#include "ftxui/dom/elements.hpp"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

enum class FileIconSize {
  Large,
  Small,
};

Element FileIcon(FileIconSize size);

}  // namespace nearby::sharing::linux_tui
