#pragma once

#include <functional>
#include <string>

#include "ftxui/component/component.hpp"
#include "sharing/linux/tui/components/share_target.h"

namespace nearby::sharing::linux_tui {
using namespace ftxui;

struct IncomingShareCardOptions {
  std::string device_name;
  ShareTargetType device_type = ShareTargetType::kUnknown;
  std::function<void()> on_accept;
  std::function<void()> on_decline;
};

Component IncomingShareCard(IncomingShareCardOptions options);

}  // namespace nearby::sharing::linux_tui
