#include "sharing/linux/app/backend.h"

#include <functional>
#include <utility>

#include <QMetaObject>
#include <QVariantMap>
#include <sdbus-c++/sdbus-c++.h>

namespace {

QString ToQString(const std::string& value) {
  return QString::fromStdString(value);
}

QVariantMap ToVariantMap(
    const nearby::sharing::linux::app::ShareTarget& target) {
  QVariantMap map;
  map.insert("id", QVariant::fromValue<qint64>(target.id));
  map.insert("deviceName", ToQString(target.device_name));
  map.insert("type", target.type);
  map.insert("isIncoming", target.is_incoming);
  map.insert("isKnown", target.is_known);
  map.insert("deviceId", ToQString(target.device_id));
  map.insert("forSelfShare", target.for_self_share);
  map.insert("vendorId", target.vendor_id);
  map.insert("receiveDisabled", target.receive_disabled);
  return map;
}

}  // namespace

Backend::Backend(QObject* parent) : QObject(parent) {
  try {
    client_ = std::make_unique<Client>(this);
    SetStatusText(QStringLiteral("Connected to nearby sharing daemon"));
  } catch (const sdbus::Error& error) {
    SetStatusText(QStringLiteral("D-Bus connection failed: %1")
                      .arg(QString::fromStdString(error.getMessage())));
  }
}

Backend::~Backend() = default;

QVariantList Backend::targets() const {
  QVariantList list;
  for (const auto& [id, target] : targets_) {
    list.push_back(ToVariantMap(target));
  }
  return list;
}

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
  targets_[target.id] = target;
  emit targetsChanged();
}

void Backend::RemoveTarget(const ShareTarget& target) {
  targets_.erase(target.id);
  emit targetsChanged();
}

void Backend::ApplyStatus(const Status& status) {
  status_ = status;
  targets_.clear();
  for (const auto& target : status.targets) {
    targets_[target.id] = target;
  }
  emit statusChanged();
  emit targetsChanged();
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
