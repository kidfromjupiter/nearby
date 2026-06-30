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

#ifndef CONNECTIONS_IMPLEMENTATION_ANALYTICS_ANALYTICS_RECORDER_IMPL_H_
#define CONNECTIONS_IMPLEMENTATION_ANALYTICS_ANALYTICS_RECORDER_IMPL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "absl/time/time.h"
#include "connections/implementation/analytics/analytics_recorder.h"
#include "location/nearby/analytics/cpp/logging/event_logger.h"
#include "proto/connections_enums.pb.h"

namespace nearby::analytics {

class AnalyticsRecorderImpl : public AnalyticsRecorder {
 public:
  explicit AnalyticsRecorderImpl(EventLogger* event_logger = nullptr)
      : event_logger_(event_logger) {}
  ~AnalyticsRecorderImpl() override = default;

  void OnStartAdvertising(
      connections::Strategy strategy,
      const std::vector<location::nearby::proto::connections::Medium>& mediums,
      AdvertisingMetadataParams* advertising_metadata_params) override {}
  void OnStopAdvertising() override {}
  int GetNextAdvertisingUpdateIndex() override { return 0; }
  void OnStartedIncomingConnectionListening(
      connections::Strategy strategy) override {}
  void OnStoppedIncomingConnectionListening() override {}
  void OnStartDiscovery(
      connections::Strategy strategy,
      const std::vector<location::nearby::proto::connections::Medium>& mediums,
      DiscoveryMetadataParams* discovery_metadata_params) override {}
  void OnStopDiscovery() override {}
  int GetNextDiscoveryUpdateIndex() override { return 0; }
  void OnEndpointFound(
      location::nearby::proto::connections::Medium medium) override {}
  void OnRequestConnection(const connections::Strategy& strategy,
                           const std::string& endpoint_id) override {}
  void OnConnectionRequestReceived(
      const std::string& remote_endpoint_id) override {}
  void OnConnectionRequestSent(const std::string& remote_endpoint_id) override {}
  void OnRemoteEndpointAccepted(
      const std::string& remote_endpoint_id) override {}
  void OnLocalEndpointAccepted(
      const std::string& remote_endpoint_id) override {}
  void OnRemoteEndpointRejected(
      const std::string& remote_endpoint_id) override {}
  void OnLocalEndpointRejected(
      const std::string& remote_endpoint_id) override {}
  void OnIncomingConnectionAttempt(
      location::nearby::proto::connections::ConnectionAttemptType type,
      location::nearby::proto::connections::Medium medium,
      location::nearby::proto::connections::ConnectionAttemptResult result,
      absl::Duration duration, const std::string& connection_token,
      ConnectionAttemptMetadataParams* connection_attempt_metadata_params)
      override {}
  void OnOutgoingConnectionAttempt(
      const std::string& remote_endpoint_id,
      location::nearby::proto::connections::ConnectionAttemptType type,
      location::nearby::proto::connections::Medium medium,
      location::nearby::proto::connections::ConnectionAttemptResult result,
      absl::Duration duration, const std::string& connection_token,
      ConnectionAttemptMetadataParams* connection_attempt_metadata_params)
      override {}
  void OnConnectionEstablished(
      const std::string& endpoint_id,
      location::nearby::proto::connections::Medium medium,
      const std::string& connection_token) override {}
  void OnConnectionClosed(
      const std::string& endpoint_id,
      location::nearby::proto::connections::Medium medium,
      location::nearby::proto::connections::DisconnectionReason reason,
      SafeDisconnectionResult result) override {}
  void OnIncomingPayloadStarted(const std::string& endpoint_id,
                                std::int64_t payload_id,
                                connections::PayloadType type,
                                std::int64_t total_size_bytes) override {}
  void OnPayloadChunkReceived(const std::string& endpoint_id,
                              std::int64_t payload_id,
                              std::int64_t chunk_size_bytes) override {}
  void OnIncomingPayloadDone(
      const std::string& endpoint_id, std::int64_t payload_id,
      location::nearby::proto::connections::PayloadStatus status,
      location::nearby::proto::connections::OperationResultCode
          operation_result_code) override {}
  void OnOutgoingPayloadStarted(const std::vector<std::string>& endpoint_ids,
                                std::int64_t payload_id,
                                connections::PayloadType type,
                                std::int64_t total_size_bytes) override {}
  void OnPayloadChunkSent(const std::string& endpoint_id,
                          std::int64_t payload_id,
                          std::int64_t chunk_size_bytes) override {}
  void OnOutgoingPayloadDone(
      const std::string& endpoint_id, std::int64_t payload_id,
      location::nearby::proto::connections::PayloadStatus status,
      location::nearby::proto::connections::OperationResultCode
          operation_result_code) override {}
  void OnBandwidthUpgradeStarted(
      const std::string& endpoint_id,
      location::nearby::proto::connections::Medium from_medium,
      location::nearby::proto::connections::Medium to_medium,
      location::nearby::proto::connections::ConnectionAttemptDirection
          direction,
      const std::string& connection_token) override {}
  void UpdateBwUpgradeNetworkInfo(const std::string& endpoint_id,
                                  int num_interfaces,
                                  int num_ipv6_only_interfaces) override {}
  void OnBandwidthUpgradeError(
      const std::string& endpoint_id,
      location::nearby::proto::connections::BandwidthUpgradeResult result,
      location::nearby::proto::connections::BandwidthUpgradeErrorStage
          error_stage,
      location::nearby::proto::connections::OperationResultCode
          operation_result_code) override {}
  void OnBandwidthUpgradeSuccess(const std::string& endpoint_id) override {}
  void OnErrorCode(const ErrorCodeParams& params) override {}
  void LogStartSession() override {}
  void LogSession() override {}
  bool IsSessionLogged() override { return false; }
  location::nearby::proto::connections::OperationResultCategory
  GetOperationResultCategory(
      location::nearby::proto::connections::OperationResultCode result_code)
      override {
    return location::nearby::proto::connections::CATEGORY_UNKNOWN;
  }
  void Sync() override {}

 private:
  EventLogger* event_logger_;
};

}  // namespace nearby::analytics

#endif  // CONNECTIONS_IMPLEMENTATION_ANALYTICS_ANALYTICS_RECORDER_IMPL_H_

