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

#include "sharing/linux/nearby_connections_adapter.h"

#include <memory>
#include <utility>

namespace nearby::sharing::linux {

CoreNearbyConnectionsAdapter::CoreNearbyConnectionsAdapter()
    : router_(std::make_unique<connections::ServiceControllerRouter>()),
      core_(std::make_unique<connections::Core>(router_.get())) {}

CoreNearbyConnectionsAdapter::~CoreNearbyConnectionsAdapter() = default;

void CoreNearbyConnectionsAdapter::StartAdvertising(
    absl::string_view service_id,
    connections::AdvertisingOptions advertising_options,
    connections::ConnectionRequestInfo request_info,
    std::function<void(connections::Status)> callback) {
  core_->StartAdvertising(service_id, std::move(advertising_options),
                          std::move(request_info), std::move(callback));
}

void CoreNearbyConnectionsAdapter::StopAdvertising(
    std::function<void(connections::Status)> callback) {
  core_->StopAdvertising(std::move(callback));
}

void CoreNearbyConnectionsAdapter::StartDiscovery(
    absl::string_view service_id,
    connections::DiscoveryOptions discovery_options,
    connections::DiscoveryListener discovery_listener,
    std::function<void(connections::Status)> callback) {
  core_->StartDiscovery(service_id, std::move(discovery_options),
                        std::move(discovery_listener), std::move(callback));
}

void CoreNearbyConnectionsAdapter::StopDiscovery(
    std::function<void(connections::Status)> callback) {
  core_->StopDiscovery(std::move(callback));
}

void CoreNearbyConnectionsAdapter::RequestConnection(
    absl::string_view endpoint_id,
    connections::ConnectionRequestInfo request_info,
    connections::ConnectionOptions connection_options,
    std::function<void(connections::Status)> callback) {
  core_->RequestConnection(endpoint_id, std::move(request_info),
                           std::move(connection_options), std::move(callback));
}

void CoreNearbyConnectionsAdapter::AcceptConnection(
    absl::string_view endpoint_id, connections::PayloadListener listener,
    std::function<void(connections::Status)> callback) {
  core_->AcceptConnection(endpoint_id, std::move(listener), std::move(callback));
}

void CoreNearbyConnectionsAdapter::RejectConnection(
    absl::string_view endpoint_id,
    std::function<void(connections::Status)> callback) {
  core_->RejectConnection(endpoint_id, std::move(callback));
}

void CoreNearbyConnectionsAdapter::SendPayload(
    absl::Span<const std::string> endpoint_ids, connections::Payload payload,
    std::function<void(connections::Status)> callback) {
  core_->SendPayload(endpoint_ids, std::move(payload), std::move(callback));
}

void CoreNearbyConnectionsAdapter::DisconnectFromEndpoint(
    absl::string_view endpoint_id,
    std::function<void(connections::Status)> callback) {
  core_->DisconnectFromEndpoint(endpoint_id, std::move(callback));
}

}  // namespace nearby::sharing::linux
