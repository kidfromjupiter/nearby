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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_GRPC_ASYNC_CLIENT_FACTORY_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_GRPC_ASYNC_CLIENT_FACTORY_H_

#include <memory>
#include <utility>

#include "internal/platform/clock.h"
#include "internal/platform/implementation/account_manager.h"
#include "sharing/analytics/analytics_recorder.h"
#include "sharing/linux/stubs/sharing_rpc_client.h"

namespace nearby::sharing::platform::common {
namespace internal {

class NoOpSharingRpcClient : public nearby::sharing::api::SharingRpcClient {
 public:
  void ListContactPeople(
      nearby::sharing::proto::ListContactPeopleRequest request,
      ListContactPeopleCallback callback) override {
    static_cast<void>(request);
    callback(nearby::sharing::proto::ListContactPeopleResponse());
  }
};

class NoOpIdentityRpcClient : public nearby::sharing::api::IdentityRpcClient {
 public:
  void QuerySharedCredentials(
      google::nearby::identity::v1::QuerySharedCredentialsRequest request,
      QuerySharedCredentialsCallback callback) override {
    static_cast<void>(request);
    callback(google::nearby::identity::v1::QuerySharedCredentialsResponse());
  }

  void PublishDevice(
      google::nearby::identity::v1::PublishDeviceRequest request,
      PublishDeviceCallback callback) override {
    static_cast<void>(request);
    callback(google::nearby::identity::v1::PublishDeviceResponse());
  }

  void GetAccountInfo(
      google::nearby::identity::v1::GetAccountInfoRequest request,
      GetAccountInfoCallback callback) override {
    static_cast<void>(request);
    callback(google::nearby::identity::v1::GetAccountInfoResponse());
  }
};

}  // namespace internal

class GrpcAsyncClientFactory {
 public:
  GrpcAsyncClientFactory(AccountManager* account_manager, Clock* clock,
                         analytics::AnalyticsRecorder* analytics_recorder) {
    static_cast<void>(account_manager);
    static_cast<void>(clock);
    static_cast<void>(analytics_recorder);
  }

  std::unique_ptr<nearby::sharing::api::SharingRpcClient> CreateInstance() {
    return std::make_unique<internal::NoOpSharingRpcClient>();
  }

  std::unique_ptr<nearby::sharing::api::IdentityRpcClient>
  CreateIdentityInstance() {
    return std::make_unique<internal::NoOpIdentityRpcClient>();
  }
};

}  // namespace nearby::sharing::platform::common

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_GRPC_ASYNC_CLIENT_FACTORY_H_
