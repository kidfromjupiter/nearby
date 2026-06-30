#include "sharing/linux/app/backend.h"

#include <algorithm>
#include <functional>
#include <thread>
#include <utility>

#include <QMetaObject>
#include <QPointer>
#include <QUrl>
#include <sdbus-c++/sdbus-c++.h>

namespace {

QString ToQString(const std::string& value) {
  return QString::fromStdString(value);
}

QString NormalizeLocalPath(const QString& path) {
  const QUrl url(path);
  if (url.isValid() && url.isLocalFile()) {
    return url.toLocalFile();
  }
  return path;
}

double ProgressFraction(const std::optional<nearby::sharing::linux::app::Transfer>& transfer) {
  if (!transfer.has_value()) {
    return 0;
  }
  return std::clamp(transfer->progress / 100.0, 0.0, 1.0);
}

bool IsAwaitingLocalConfirmation(
    const std::optional<nearby::sharing::linux::app::Transfer>& transfer) {
  return transfer.has_value() &&
         transfer->status == "kAwaitingLocalConfirmation";
}

QString DeviceNameFor(
    const std::optional<nearby::sharing::linux::app::ShareTarget>& target) {
  if (!target.has_value() || target->device_name.empty()) {
    return QStringLiteral("Unknown device");
  }
  return ToQString(target->device_name);
}

}  // namespace

ShareTargetModel::ShareTargetModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ShareTargetModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(targets_.size());
}

QVariant ShareTargetModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(targets_.size())) {
    return {};
  }

  const ShareTarget& target = targets_[index.row()];
  switch (role) {
    case IdRole:
      return QVariant::fromValue<qint64>(target.id);
    case DeviceNameRole:
      return ToQString(target.device_name);
    case TypeRole:
      return target.type;
    case IsIncomingRole:
      return target.is_incoming;
    case IsKnownRole:
      return target.is_known;
    case DeviceIdRole:
      return ToQString(target.device_id);
    case ForSelfShareRole:
      return target.for_self_share;
    case VendorIdRole:
      return target.vendor_id;
    case ReceiveDisabledRole:
      return target.receive_disabled;
    default:
      return {};
  }
}

QHash<int, QByteArray> ShareTargetModel::roleNames() const {
  return {
      {IdRole, "targetId"},
      {DeviceNameRole, "deviceName"},
      {TypeRole, "type"},
      {IsIncomingRole, "isIncoming"},
      {IsKnownRole, "isKnown"},
      {DeviceIdRole, "deviceId"},
      {ForSelfShareRole, "forSelfShare"},
      {VendorIdRole, "vendorId"},
      {ReceiveDisabledRole, "receiveDisabled"},
  };
}

void ShareTargetModel::ApplyTarget(const ShareTarget& target) {
  const int row = IndexOf(target.id);
  if (row < 0) {
    const int insert_row = static_cast<int>(targets_.size());
    beginInsertRows(QModelIndex(), insert_row, insert_row);
    targets_.push_back(target);
    endInsertRows();
    return;
  }

  targets_[row] = target;
  const QModelIndex changed_index = index(row);
  emit dataChanged(changed_index, changed_index,
                   {IdRole, DeviceNameRole, TypeRole, IsIncomingRole,
                    IsKnownRole, DeviceIdRole, ForSelfShareRole, VendorIdRole,
                    ReceiveDisabledRole});
}

void ShareTargetModel::RemoveTarget(int64_t target_id) {
  const int row = IndexOf(target_id);
  if (row < 0) {
    return;
  }

  beginRemoveRows(QModelIndex(), row, row);
  targets_.erase(targets_.begin() + row);
  endRemoveRows();
}

void ShareTargetModel::ResetTargets(const std::vector<ShareTarget>& targets) {
  beginResetModel();
  targets_ = targets;
  endResetModel();
}

std::optional<ShareTargetModel::ShareTarget> ShareTargetModel::FindTarget(
    int64_t target_id) const {
  const int row = IndexOf(target_id);
  if (row < 0) {
    return std::nullopt;
  }
  return targets_[row];
}

int ShareTargetModel::IndexOf(int64_t target_id) const {
  for (int i = 0; i < static_cast<int>(targets_.size()); ++i) {
    if (targets_[i].id == target_id) {
      return i;
    }
  }
  return -1;
}

ShareTransferModel::ShareTransferModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ShareTransferModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(transfers_.size());
}

QVariant ShareTransferModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(transfers_.size())) {
    return {};
  }
  return DataForRow(transfers_[index.row()], role);
}

QHash<int, QByteArray> ShareTransferModel::roleNames() const {
  return {
      {IdRole, "transferId"},
      {DirectionRole, "direction"},
      {DeviceNameRole, "deviceName"},
      {TypeRole, "type"},
      {StatusRole, "status"},
      {ProgressRole, "progress"},
      {TransferredBytesRole, "transferredBytes"},
      {TotalBytesRole, "totalBytes"},
      {TransferSpeedRole, "transferSpeed"},
      {EstimatedTimeRemainingRole, "estimatedTimeRemaining"},
      {TotalAttachmentsCountRole, "totalAttachmentsCount"},
      {TransferredAttachmentsCountRole, "transferredAttachmentsCount"},
      {IsFinalStatusRole, "isFinalStatus"},
      {HasTargetRole, "hasTarget"},
      {HasTransferRole, "hasTransfer"},
      {AwaitingLocalConfirmationRole, "awaitingLocalConfirmation"},
      {LocalPathRole, "localPath"},
  };
}

void ShareTransferModel::ApplyTarget(const ShareTarget& target) {
  const int row = IndexOf(target.id);
  if (row < 0) {
    return;
  }
  transfers_[row].target = target;
  EmitRowChanged(row);
}

void ShareTransferModel::ApplyTransfer(const QString& direction,
                                       const ShareTarget& target,
                                       const Transfer& transfer) {
  int row = IndexOf(target.id);
  if (row < 0) {
    row = static_cast<int>(transfers_.size());
    beginInsertRows(QModelIndex(), row, row);
    transfers_.push_back(Row{target.id, direction, target, transfer, {}});
    endInsertRows();
    return;
  }

  transfers_[row].direction = direction;
  transfers_[row].target = target;
  transfers_[row].transfer = transfer;
  EmitRowChanged(row);
}

void ShareTransferModel::PrepareOutgoingTransfer(
    int64_t target_id, const QString& local_path,
    const std::optional<ShareTarget>& target) {
  int row = IndexOf(target_id);
  if (row < 0) {
    row = static_cast<int>(transfers_.size());
    beginInsertRows(QModelIndex(), row, row);
    Row transfer;
    transfer.id = target_id;
    transfer.direction = QStringLiteral("send");
    transfer.target = target;
    transfer.local_path = local_path;
    transfers_.push_back(std::move(transfer));
    endInsertRows();
    return;
  }

  transfers_[row].direction = QStringLiteral("send");
  transfers_[row].target = target;
  transfers_[row].local_path = local_path;
  EmitRowChanged(row);
}

void ShareTransferModel::RemoveTransfer(int64_t target_id) {
  const int row = IndexOf(target_id);
  if (row < 0) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  transfers_.erase(transfers_.begin() + row);
  endRemoveRows();
}

int ShareTransferModel::IndexOf(int64_t target_id) const {
  for (int i = 0; i < static_cast<int>(transfers_.size()); ++i) {
    if (transfers_[i].id == target_id) {
      return i;
    }
  }
  return -1;
}

QVariant ShareTransferModel::DataForRow(const Row& row, int role) const {
  const Transfer empty_transfer;
  const Transfer& transfer = row.transfer.value_or(empty_transfer);

  switch (role) {
    case IdRole:
      return QVariant::fromValue<qint64>(row.id);
    case DirectionRole:
      return row.direction;
    case DeviceNameRole:
      return DeviceNameFor(row.target);
    case TypeRole:
      return row.target.has_value() ? row.target->type : 0;
    case StatusRole:
      if (row.transfer.has_value()) {
        return ToQString(transfer.status);
      }
      return row.direction == QStringLiteral("send")
                 ? QStringLiteral("kConnecting")
                 : QStringLiteral("kUnknown");
    case ProgressRole:
      return ProgressFraction(row.transfer);
    case TransferredBytesRole:
      return QVariant::fromValue<qint64>(transfer.transferred_bytes);
    case TotalBytesRole:
      return QVariant::fromValue<qint64>(transfer.total_bytes);
    case TransferSpeedRole:
      return QVariant::fromValue<qint64>(transfer.transfer_speed);
    case EstimatedTimeRemainingRole:
      return QVariant::fromValue<qint64>(transfer.estimated_time_remaining);
    case TotalAttachmentsCountRole:
      return transfer.total_attachments_count;
    case TransferredAttachmentsCountRole:
      return transfer.transferred_attachments_count;
    case IsFinalStatusRole:
      return transfer.is_final_status;
    case HasTargetRole:
      return row.target.has_value();
    case HasTransferRole:
      return row.transfer.has_value();
    case AwaitingLocalConfirmationRole:
      return IsAwaitingLocalConfirmation(row.transfer);
    case LocalPathRole:
      return row.local_path;
    default:
      return {};
  }
}

void ShareTransferModel::EmitRowChanged(int row) {
  const QModelIndex changed_index = index(row);
  emit dataChanged(changed_index, changed_index,
                   {IdRole, DirectionRole, DeviceNameRole, TypeRole,
                    StatusRole, ProgressRole, TransferredBytesRole,
                    TotalBytesRole, TransferSpeedRole,
                    EstimatedTimeRemainingRole, TotalAttachmentsCountRole,
                    TransferredAttachmentsCountRole, IsFinalStatusRole,
                    HasTargetRole, HasTransferRole,
                    AwaitingLocalConfirmationRole, LocalPathRole});
}

Backend::Backend(QObject* parent)
    : QObject(parent), targets_(this), transfers_(this) {
  try {
    client_ = std::make_unique<Client>(this);
    SetStatusText(QStringLiteral("Connected to nearby sharing daemon"));
  } catch (const sdbus::Error& error) {
    SetStatusText(QStringLiteral("D-Bus connection failed: %1")
                      .arg(QString::fromStdString(error.getMessage())));
  }
}

Backend::~Backend() = default;

void Backend::startReceive() {
  RunCommand(QStringLiteral("Start receive"),
             [this]() { return client_->StartReceive(); });
}

void Backend::stopReceive() {
  RunCommand(QStringLiteral("Stop receive"),
             [this]() { return client_->StopReceive(); });
}

void Backend::startDiscovery() {
  RunCommand(QStringLiteral("Start discovery"),
             [this]() { return client_->StartDiscovery(); });
}

void Backend::stopDiscovery() {
  RunCommand(QStringLiteral("Stop discovery"),
             [this]() { return client_->StopDiscovery(); });
}

void Backend::sendFile(qint64 share_target_id, const QString& path) {
  const QString local_path = NormalizeLocalPath(path);
  RunCommand(QStringLiteral("Send file"), [this, share_target_id, local_path]() {
    return client_->SendFile(share_target_id, local_path.toStdString());
  });
}

void Backend::prepareOutgoingTransfer(qint64 share_target_id,
                                      const QString& path) {
  transfers_.PrepareOutgoingTransfer(share_target_id, NormalizeLocalPath(path),
                                     targets_.FindTarget(share_target_id));
}

void Backend::accept(qint64 share_target_id) {
  RunCommand(QStringLiteral("Accept transfer"), [this, share_target_id]() {
    return client_->Accept(share_target_id);
  });
}

void Backend::reject(qint64 share_target_id) {
  RunCommand(QStringLiteral("Reject transfer"), [this, share_target_id]() {
    return client_->Reject(share_target_id);
  });
}

void Backend::cancel(qint64 share_target_id) {
  RunCommand(QStringLiteral("Cancel transfer"), [this, share_target_id]() {
    return client_->Cancel(share_target_id);
  });
}

void Backend::OnTargetDiscovered(const ShareTarget& target) {
  QMetaObject::invokeMethod(
      this, [this, target]() { ApplyTarget(target); }, Qt::QueuedConnection);
}

void Backend::OnTargetUpdated(const ShareTarget& target) {
  QMetaObject::invokeMethod(
      this, [this, target]() { ApplyTarget(target); }, Qt::QueuedConnection);
}

void Backend::OnTargetLost(const ShareTarget& target) {
  QMetaObject::invokeMethod(
      this, [this, target]() { RemoveTarget(target); }, Qt::QueuedConnection);
}

void Backend::OnIncomingTransfer(const std::string& direction,
                                 const ShareTarget& target,
                                 const Transfer& transfer) {
  QMetaObject::invokeMethod(
      this,
      [this, direction, target, transfer]() {
        ApplyTarget(target);
        transfers_.ApplyTransfer(ToQString(direction), target, transfer);
        emit incomingTransfer(target.id, ToQString(target.device_name),
                              ToQString(transfer.status));
      },
      Qt::QueuedConnection);
}

void Backend::OnTransferUpdate(const std::string& direction,
                               const ShareTarget& target,
                               const Transfer& transfer) {
  QMetaObject::invokeMethod(
      this,
      [this, direction, target, transfer]() {
        ApplyTarget(target);
        transfers_.ApplyTransfer(ToQString(direction), target, transfer);
        emit transferUpdate(target.id, ToQString(target.device_name),
                            ToQString(transfer.status), transfer.progress);
      },
      Qt::QueuedConnection);
}

void Backend::OnStatusChanged(const Status& status) {
  QMetaObject::invokeMethod(
      this, [this, status]() { ApplyStatus(status); }, Qt::QueuedConnection);
}

void Backend::ApplyTarget(const ShareTarget& target) {
  targets_.ApplyTarget(target);
  transfers_.ApplyTarget(target);
}

void Backend::RemoveTarget(const ShareTarget& target) {
  targets_.RemoveTarget(target.id);
}

void Backend::ApplyStatus(const Status& status) {
  status_ = status;
  targets_.ResetTargets(status.targets);
  emit statusChanged();
}

void Backend::ApplyCommandResult(const QString& command,
                                 const std::tuple<bool, std::string>& result) {
  const auto& [ok, message] = result;
  const QString detail =
      message.empty() ? (ok ? QStringLiteral("ok") : QStringLiteral("failed"))
                      : ToQString(message);
  SetStatusText(QStringLiteral("%1: %2").arg(command, detail));
}

void Backend::RunCommand(
    const QString& command,
    const std::function<std::tuple<bool, std::string>()>& operation) {
  if (!client_) {
    SetStatusText(QStringLiteral("%1 failed: not connected").arg(command));
    return;
  }

  QPointer<Backend> backend(this);
  std::thread([backend, command, operation]() {
    try {
      auto result = operation();
      if (!backend) {
        return;
      }
      QMetaObject::invokeMethod(
          backend,
          [backend, command, result = std::move(result)]() {
            if (!backend) {
              return;
            }
            backend->ApplyCommandResult(command, result);
          },
          Qt::QueuedConnection);
    } catch (const sdbus::Error& error) {
      const QString message = QStringLiteral("%1 failed: %2")
                                  .arg(command, QString::fromStdString(
                                                    error.getMessage()));
      if (!backend) {
        return;
      }
      QMetaObject::invokeMethod(
          backend,
          [backend, message]() {
            if (!backend) {
              return;
            }
            backend->SetStatusText(message);
          },
          Qt::QueuedConnection);
    }
  }).detach();
}


void Backend::SetStatusText(const QString& text) {
  if (status_text_ == text) {
    return;
  }
  status_text_ = text;
  emit statusTextChanged();
}
