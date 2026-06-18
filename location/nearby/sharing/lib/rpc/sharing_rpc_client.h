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

#ifndef LOCATION_NEARBY_SHARING_LIB_RPC_SHARING_RPC_CLIENT_H_
#define LOCATION_NEARBY_SHARING_LIB_RPC_SHARING_RPC_CLIENT_H_

#include <functional>

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "location/nearby/sharing/lib/rpc/identity_rpc_types.h"
#include "sharing/proto/contact_rpc.pb.h"

namespace nearby::sharing::api {

class SharingRpcClient {
 public:
  using ListContactPeopleCallback = std::function<void(
      const absl::StatusOr<nearby::sharing::proto::ListContactPeopleResponse>&)>;

  virtual ~SharingRpcClient() = default;

  virtual void ListContactPeople(
      nearby::sharing::proto::ListContactPeopleRequest request,
      ListContactPeopleCallback callback) = 0;
};

class IdentityRpcClient {
 public:
  static constexpr absl::Duration kTimeout = absl::Seconds(30);

  using QuerySharedCredentialsCallback = std::function<void(
      const absl::StatusOr<
          google::nearby::identity::v1::QuerySharedCredentialsResponse>&)>;
  using QuerySharedCredentialsWithBindingIdsCallback = std::function<void(
      const absl::StatusOr<google::nearby::identity::v1::
                               QuerySharedCredentialsWithBindingIdsResponse>&)>;
  using PublishDeviceCallback = std::function<void(
      const absl::StatusOr<google::nearby::identity::v1::PublishDeviceResponse>&)>;
  using GetAccountInfoCallback = std::function<void(
      const absl::StatusOr<
          google::nearby::identity::v1::GetAccountInfoResponse>&)>;

  virtual ~IdentityRpcClient() = default;

  virtual void QuerySharedCredentials(
      google::nearby::identity::v1::QuerySharedCredentialsRequest request,
      absl::Duration timeout, QuerySharedCredentialsCallback callback) = 0;
  virtual void QuerySharedCredentialsWithBindingIds(
      google::nearby::identity::v1::QuerySharedCredentialsWithBindingIdsRequest
          request,
      absl::Duration timeout,
      QuerySharedCredentialsWithBindingIdsCallback callback) = 0;
  virtual void PublishDevice(
      google::nearby::identity::v1::PublishDeviceRequest request,
      absl::Duration timeout, PublishDeviceCallback callback) = 0;
  virtual void GetAccountInfo(
      google::nearby::identity::v1::GetAccountInfoRequest request,
      absl::Duration timeout, GetAccountInfoCallback callback) = 0;
};

}  // namespace nearby::sharing::api

#endif  // LOCATION_NEARBY_SHARING_LIB_RPC_SHARING_RPC_CLIENT_H_

