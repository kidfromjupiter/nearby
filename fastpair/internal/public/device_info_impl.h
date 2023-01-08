// Copyright 2022 Google LLC
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

#ifndef THIRD_PARTY_NEARBY_FASTPAIR_INTERNAL_PUBLIC_DEVICE_INFO_IMPL_H_
#define THIRD_PARTY_NEARBY_FASTPAIR_INTERNAL_PUBLIC_DEVICE_INFO_IMPL_H_

#include <functional>
#include <memory>
#include <string>

#include "fastpair/internal/api/fast_pair_platform.h"
#include "fastpair/internal/public/device_info.h"

namespace nearby {
namespace fastpair {

class DeviceInfoImpl : public DeviceInfo {
 public:
  DeviceInfoImpl()
      : device_info_impl_(
            api::ImplementationFastPairPlatform::CreateDeviceInfo()) {}

  api::DeviceInfo::OsType GetOsType() const override;

  bool IsScreenLocked() const override;
  void RegisterScreenLockedListener(
      absl::string_view listener_name,
      std::function<void(api::DeviceInfo::ScreenStatus)> callback) override;
  void UnregisterScreenLockedListener(absl::string_view listener_name) override;

 private:
  std::unique_ptr<api::DeviceInfo> device_info_impl_;
};

}  // namespace fastpair
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_FASTPAIR_INTERNAL_PUBLIC_DEVICE_INFO_IMPL_H_