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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_HIGHWAY_FINGERPRINT_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_HIGHWAY_FINGERPRINT_H_

#include <cstdint>

#include "absl/hash/hash.h"
#include "absl/strings/string_view.h"

namespace util_hash {

inline uint64_t HighwayFingerprint64(absl::string_view input) {
  return absl::Hash<absl::string_view>{}(input);
}

}  // namespace util_hash

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_HIGHWAY_FINGERPRINT_H_
