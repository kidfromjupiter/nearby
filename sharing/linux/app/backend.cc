#include "sharing/linux/app/backend.h"

#include <functional>
#include <utility>

#include <QMetaObject>
#include <sdbus-c++/sdbus-c++.h>

namespace {

QString ToQString(const std::string& value) {
  return QString::fromStdString(value);
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
      {IdRole, "id"},
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

int ShareTargetModel::IndexOf(int64_t target_id) const {
  for (int i = 0; i < static_cast<int>(targets_.size()); ++i) {
    if (targets_[i].id == target_id) {
      return i;
    }
  }
  return -1;
}

Backend::Backend(QObject* parent) : QObject(parent), targets_(this) {
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
  RunCommand(QStringLiteral("Send file"), [this, share_target_id, path]() {
    return client_->SendFile(share_target_id, path.toStdString());
  });
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
      [this, target, transfer]() {
        ApplyTarget(target);
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
      [this, target, transfer]() {
        ApplyTarget(target);
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

  try {
    ApplyCommandResult(command, operation());
  } catch (const sdbus::Error& error) {
    SetStatusText(
        QStringLiteral("%1 failed: %2")
            .arg(command, QString::fromStdString(error.getMessage())));
  }
}


void Backend::SetStatusText(const QString& text) {
  if (status_text_ == text) {
    return;
  }
  status_text_ = text;
  emit statusTextChanged();
}
