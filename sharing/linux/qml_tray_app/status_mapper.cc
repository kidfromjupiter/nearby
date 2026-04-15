#include "status_mapper.h"

namespace StatusMapper {

QString TransferStatusToString(NearbySharingApi::TransferStatus status) {
  return QString::fromStdString(NearbySharingApi::TransferStatusToString(status));
}

QString ApiStatusToString(NearbySharingApi::StatusCode status) {
  return QString::fromStdString(NearbySharingApi::StatusCodeToString(status));
}

bool IsActiveTransferStatus(const QString& status) {
  return status == QStringLiteral("Queued") ||
         status == QStringLiteral("Connecting") ||
         status == QStringLiteral("AwaitingLocalConfirmation") ||
         status == QStringLiteral("AwaitingRemoteAcceptance") ||
         status == QStringLiteral("InProgress");
}

bool IsFinalTransferStatus(NearbySharingApi::TransferStatus status) {
  switch (status) {
    case NearbySharingApi::TransferStatus::kComplete:
    case NearbySharingApi::TransferStatus::kFailed:
    case NearbySharingApi::TransferStatus::kRejected:
    case NearbySharingApi::TransferStatus::kCancelled:
    case NearbySharingApi::TransferStatus::kTimedOut:
    case NearbySharingApi::TransferStatus::kMediaUnavailable:
    case NearbySharingApi::TransferStatus::kNotEnoughSpace:
    case NearbySharingApi::TransferStatus::kUnsupportedAttachmentType:
    case NearbySharingApi::TransferStatus::kDeviceAuthenticationFailed:
    case NearbySharingApi::TransferStatus::kIncompletePayloads:
      return true;
    case NearbySharingApi::TransferStatus::kUnknown:
    case NearbySharingApi::TransferStatus::kConnecting:
    case NearbySharingApi::TransferStatus::kAwaitingLocalConfirmation:
    case NearbySharingApi::TransferStatus::kAwaitingRemoteAcceptance:
    case NearbySharingApi::TransferStatus::kInProgress:
      return false;
  }
  return false;
}

}  // namespace StatusMapper
