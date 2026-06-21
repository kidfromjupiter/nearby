#pragma once

#include <string>

#include "ftxui/dom/elements.hpp"

namespace nearby::sharing::linux_tui {

struct SidebarState {
  std::string hostname;
  std::string selected_file;
};

ftxui::Element Sidebar(const SidebarState& state);

}  // namespace nearby::sharing::linux_tui
