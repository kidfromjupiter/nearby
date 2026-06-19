// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/platform/implementation/linux/ble_l2cap_socket.h"

#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "internal/platform/implementation/crypto.h"
#include "internal/platform/logging.h"
#include "proto/mediums/ble_frames.pb.h"

namespace nearby {
namespace linux {

// Migrated from bespoke l2cap socket stream semantics to linux platform stream
// semantics
//
}  // namespace linux
}  // namespace nearby
