// Copyright 2026 Google LLC
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

#ifndef SHARING_LINUX_PLATFORM_PLATFORM_UTIL_H_
#define SHARING_LINUX_PLATFORM_PLATFORM_UTIL_H_

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>

#include "internal/base/file_path.h"
#include "internal/platform/implementation/linux/bluetooth_adapter.h"

namespace nearby::sharing::linux::internal {

std::string GetEnvOrDefault(const char* key, std::string fallback);
std::string GetHomeDirectory();
FilePath BuildPathFromBase(const std::string& base,
                           std::initializer_list<std::string> components);
std::optional<std::string> GetLanguageCode();
bool HasNonLoopbackInterface();

std::shared_ptr<::nearby::linux::BluetoothAdapter>
CreateFastInitBluetoothAdapter();

}  // namespace nearby::sharing::linux::internal

#endif  // SHARING_LINUX_PLATFORM_PLATFORM_UTIL_H_
