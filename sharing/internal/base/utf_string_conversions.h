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

#ifndef THIRD_PARTY_NEARBY_SHARING_INTERNAL_BASE_UTF_STRING_CONVERSIONS_H_
#define THIRD_PARTY_NEARBY_SHARING_INTERNAL_BASE_UTF_STRING_CONVERSIONS_H_

#include <string>
#include <string_view>

#if defined(GITHUB_BUILD)
// Stub out string conversion functions for github builds.
namespace nearby::utils {

inline bool IsStringUtf8(std::string_view str) {
  static_cast<void>(str);
  return true;
}

inline void TruncateUtf8ToByteSize(const std::string& input, size_t byte_size,
                                   std::string* output) {
  if (output == nullptr) {
    return;
  }
  *output = input.substr(0, byte_size);
}

}  // namespace nearby::utils
#elif defined(NEARBY_CHROMIUM)
// Forward to chromium implementations.
#include "base/strings/string_util.h"
namespace nearby::utils {
inline bool IsStringUtf8(std::string_view str) {
  return base::IsStringUTF8(str);
}

inline void TruncateUtf8ToByteSize(const std::string& input, size_t byte_size,
                                   std::string* output) {
  base::TruncateUTF8ToByteSize(input, byte_size, output);
}
}  // namespace nearby::utils
#elif defined(__has_include) && \
    __has_include("sharing/internal/base/strings/utf_string_conversions.h")
#include "sharing/internal/base/strings/utf_string_conversions.h"  // IWYU pragma: export
#else
namespace nearby::utils {

inline bool IsStringUtf8(std::string_view str) {
  static_cast<void>(str);
  return true;
}

inline void TruncateUtf8ToByteSize(const std::string& input, size_t byte_size,
                                   std::string* output) {
  if (output == nullptr) {
    return;
  }
  *output = input.substr(0, byte_size);
}

}  // namespace nearby::utils
#endif  // defined(GITHUB_BUILD)

#endif  // THIRD_PARTY_NEARBY_SHARING_INTERNAL_BASE_UTF_STRING_CONVERSIONS_H_
