#ifndef SHARING_LINUX_QML_TRAY_APP_STATUS_MAPPER_H_
#define SHARING_LINUX_QML_TRAY_APP_STATUS_MAPPER_H_

#include <QString>
#include "sharing/linux/nearby_sharing_api.h"

using NearbySharingApi = nearby::sharing::NearbySharingApi;

namespace StatusMapper {

QString TransferStatusToString(NearbySharingApi::TransferStatus status);

QString ApiStatusToString(NearbySharingApi::StatusCode status);

bool IsActiveTransferStatus(const QString& status);

bool IsFinalTransferStatus(NearbySharingApi::TransferStatus status);

}  // namespace StatusMapper

#endif  // SHARING_LINUX_QML_TRAY_APP_STATUS_MAPPER_H_
