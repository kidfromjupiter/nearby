// Copyright 2025 Google LLC
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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_CONNECTIONS_ADAPTER_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_CONNECTIONS_ADAPTER_H_

#include <functional>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "connections/advertising_options.h"
#include "connections/connection_options.h"
#include "connections/core.h"
#include "connections/discovery_options.h"
#include "connections/implementation/service_controller_router.h"
#include "connections/listeners.h"
#include "connections/payload.h"
#include "connections/status.h"

namespace nearby::sharing::linux {

class NearbyConnectionsAdapter {
 public:
  virtual ~NearbyConnectionsAdapter() = default;

  virtual void StartAdvertising(
      absl::string_view service_id,
      connections::AdvertisingOptions advertising_options,
      connections::ConnectionRequestInfo request_info,
      std::function<void(connections::Status)> callback) = 0;
  virtual void StopAdvertising(
      std::function<void(connections::Status)> callback) = 0;
  virtual void StartDiscovery(
      absl::string_view service_id,
      connections::DiscoveryOptions discovery_options,
      connections::DiscoveryListener discovery_listener,
      std::function<void(connections::Status)> callback) = 0;
  virtual void StopDiscovery(
      std::function<void(connections::Status)> callback) = 0;
  virtual void RequestConnection(
      absl::string_view endpoint_id,
      connections::ConnectionRequestInfo request_info,
      connections::ConnectionOptions connection_options,
      std::function<void(connections::Status)> callback) = 0;
  virtual void AcceptConnection(
      absl::string_view endpoint_id, connections::PayloadListener listener,
      std::function<void(connections::Status)> callback) = 0;
  virtual void RejectConnection(
      absl::string_view endpoint_id,
      std::function<void(connections::Status)> callback) = 0;
  virtual void SendPayload(
      absl::Span<const std::string> endpoint_ids, connections::Payload payload,
      std::function<void(connections::Status)> callback) = 0;
  virtual void DisconnectFromEndpoint(
      absl::string_view endpoint_id,
      std::function<void(connections::Status)> callback) = 0;
};

class CoreNearbyConnectionsAdapter : public NearbyConnectionsAdapter {
 public:
  CoreNearbyConnectionsAdapter();
  ~CoreNearbyConnectionsAdapter() override;

  void StartAdvertising(
      absl::string_view service_id,
      connections::AdvertisingOptions advertising_options,
      connections::ConnectionRequestInfo request_info,
      std::function<void(connections::Status)> callback) override;
  void StopAdvertising(
      std::function<void(connections::Status)> callback) override;
  void StartDiscovery(
      absl::string_view service_id,
      connections::DiscoveryOptions discovery_options,
      connections::DiscoveryListener discovery_listener,
      std::function<void(connections::Status)> callback) override;
  void StopDiscovery(
      std::function<void(connections::Status)> callback) override;
  void RequestConnection(
      absl::string_view endpoint_id,
      connections::ConnectionRequestInfo request_info,
      connections::ConnectionOptions connection_options,
      std::function<void(connections::Status)> callback) override;
  void AcceptConnection(
      absl::string_view endpoint_id, connections::PayloadListener listener,
      std::function<void(connections::Status)> callback) override;
  void RejectConnection(
      absl::string_view endpoint_id,
      std::function<void(connections::Status)> callback) override;
  void SendPayload(
      absl::Span<const std::string> endpoint_ids, connections::Payload payload,
      std::function<void(connections::Status)> callback) override;
  void DisconnectFromEndpoint(
      absl::string_view endpoint_id,
      std::function<void(connections::Status)> callback) override;

 private:
  std::unique_ptr<connections::ServiceControllerRouter> router_;
  std::unique_ptr<connections::Core> core_;
};

}  // namespace nearby::sharing::linux

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_NEARBY_CONNECTIONS_ADAPTER_H_
