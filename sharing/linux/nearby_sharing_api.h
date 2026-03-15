// Copyright 2026
//
// Thin app-facing API for NearbySharingServiceLinux that avoids exposing
// internal Nearby headers to external consumers.

#ifndef SHARING_LINUX_NEARBY_SHARING_API_H_
#define SHARING_LINUX_NEARBY_SHARING_API_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Some toolchains define `linux` as a macro (e.g. `#define linux 1`), which
// breaks namespace tokens like `nearby::sharing::linux`.
#ifdef linux
#undef linux
#endif

namespace nearby {
namespace sharing {
namespace linux {

class __attribute__((visibility("default"))) NearbySharingApi {
 public:
  enum class StatusCode {
    kOk = 0,
    kError = 1,
    kOutOfOrderApiCall = 2,
    kStatusAlreadyStopped = 3,
    kTransferAlreadyInProgress = 4,
    kNoAvailableConnectionMedium = 5,
    kIrrecoverableHardwareError = 6,
    kInvalidArgument = 7,
  };

  enum class TransferStatus {
    kUnknown = 0,
    kConnecting = 1,
    kAwaitingLocalConfirmation = 2,
    kAwaitingRemoteAcceptance = 3,
    kInProgress = 4,
    kComplete = 5,
    kFailed = 6,
    kRejected = 7,
    kCancelled = 8,
    kTimedOut = 9,
    kMediaUnavailable = 10,
    kNotEnoughSpace = 11,
    kUnsupportedAttachmentType = 12,
    kDeviceAuthenticationFailed = 13,
    kIncompletePayloads = 14,
  };

  enum class TextAttachmentType {
    kUnknown = 0,
    kText = 1,
    kUrl = 2,
    kPhoneNumber = 3,
    kAddress = 4,
  };

  struct ShareTargetInfo {
    int64_t id = 0;
    std::string device_name;
    bool is_incoming = false;
    int device_type = 0;
  };

  struct TextAttachmentInfo {
    TextAttachmentType type = TextAttachmentType::kUnknown;
    std::string text_title;
    std::string text_body;
  };

  struct TransferUpdateInfo {
    int64_t share_target_id = 0;
    std::string device_name;
    bool is_incoming = false;
    TransferStatus status = TransferStatus::kUnknown;
    float progress = 0.0f;
    uint64_t transferred_bytes = 0;
    int total_attachments = 0;
    int transferred_attachments = 0;
    std::string first_file_name;
    std::string first_file_path;
    std::vector<TextAttachmentInfo> text_attachments;
  };

  struct Listener {
    std::function<void(const ShareTargetInfo&)> target_discovered_cb;
    std::function<void(const ShareTargetInfo&)> target_updated_cb;
    std::function<void(int64_t)> target_lost_cb;
    std::function<void(const TransferUpdateInfo&)> transfer_update_cb;
  };

  NearbySharingApi();
  explicit NearbySharingApi(std::string device_name_override);
  ~NearbySharingApi();

  NearbySharingApi(const NearbySharingApi&) = delete;
  NearbySharingApi& operator=(const NearbySharingApi&) = delete;
  NearbySharingApi(NearbySharingApi&&) noexcept;
  NearbySharingApi& operator=(NearbySharingApi&&) noexcept;

  void SetListener(Listener listener);

  void StartSendMode(std::function<void(StatusCode)> callback);
  void StopSendMode(std::function<void(StatusCode)> callback);

  void StartReceiveMode(std::function<void(StatusCode)> callback);
  void StopReceiveMode(std::function<void(StatusCode)> callback);

  void SendFile(int64_t share_target_id, const std::string& file_path,
                std::function<void(StatusCode)> callback);
  void Accept(int64_t share_target_id, std::function<void(StatusCode)> callback);
  void Reject(int64_t share_target_id, std::function<void(StatusCode)> callback);
  void Cancel(int64_t share_target_id, std::function<void(StatusCode)> callback);

  void Shutdown(std::function<void(StatusCode)> callback);
  std::string GetQrCodeUrl() const;

  static std::string StatusCodeToString(StatusCode status);
  static std::string TransferStatusToString(TransferStatus status);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace linux
}  // namespace sharing
}  // namespace nearby

#endif  // SHARING_LINUX_NEARBY_SHARING_API_H_
