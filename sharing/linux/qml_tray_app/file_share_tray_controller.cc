#include "file_share_tray_controller.h"

#include <QByteArray>
#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>
#include <QVariantMap>

#include "third_party/libqrencode/qrencode_compat.h"

namespace {

QString TrimmedOrFallback(const QString& value, const QString& fallback) {
  const QString trimmed = value.trimmed();
  return trimmed.isEmpty() ? fallback : trimmed;
}

QString TrimmedStdString(const std::string& value) {
  return QString::fromStdString(value).trimmed();
}

QString FirstReceivedUrl(const NearbySharingApi::TransferUpdateInfo& update) {
  for (const NearbySharingApi::TextAttachmentInfo& text :
       update.text_attachments) {
    if (text.type != NearbySharingApi::TextAttachmentType::kUrl) {
      continue;
    }

    const QString link = TrimmedStdString(text.text_body);
    if (!link.isEmpty()) {
      return link;
    }
  }

  return {};
}

QString FirstReceivedTextSummary(
    const NearbySharingApi::TransferUpdateInfo& update) {
  for (const NearbySharingApi::TextAttachmentInfo& text :
       update.text_attachments) {
    const QString title = TrimmedStdString(text.text_title);
    if (!title.isEmpty()) {
      return title;
    }

    const QString body = TrimmedStdString(text.text_body);
    if (!body.isEmpty()) {
      return body;
    }
  }

  return {};
}

}  // namespace

FileShareTrayController::FileShareTrayController(QObject* parent)
    : QObject(parent) {
  const QString host = QSysInfo::machineHostName().trimmed();
  if (!host.isEmpty()) {
    device_name_ = host;
  }

  LoadSettings();
  CreateService();
}

FileShareTrayController::~FileShareTrayController() {
  stop();
  if (service_) {
    service_->Shutdown([](NearbySharingApi::StatusCode) {});
  }
}

void FileShareTrayController::CreateService() {
  service_ = std::make_unique<NearbySharingApi>(device_name_.toStdString());
  qr_code_url_ = QString::fromStdString(service_->GetQrCodeUrl());
  UpdateQrCodeData();
  emit qrCodeUrlChanged();
  emit qrCodeChanged();
  AttachServiceListeners();
}

void FileShareTrayController::UpdateQrCodeData() {
  qr_code_rows_.clear();
  qr_code_size_ = 0;

  const QByteArray encoded_url = qr_code_url_.trimmed().toUtf8();
  if (encoded_url.isEmpty()) {
    return;
  }

  QRcode* qr_code = QRcode_encodeData(
      encoded_url.size(),
      reinterpret_cast<const unsigned char*>(encoded_url.constData()), 0,
      QR_ECLEVEL_M);
  if (qr_code == nullptr || qr_code->data == nullptr || qr_code->width <= 0) {
    if (qr_code != nullptr) {
      QRcode_free(qr_code);
    }
    return;
  }

  qr_code_size_ = qr_code->width;
  qr_code_rows_.reserve(qr_code_size_);

  for (int row = 0; row < qr_code_size_; ++row) {
    QString row_data;
    row_data.reserve(qr_code_size_);

    for (int col = 0; col < qr_code_size_; ++col) {
      const unsigned char module =
          qr_code->data[row * qr_code_size_ + col] & 0x1;
      row_data.append(module ? QLatin1Char('1') : QLatin1Char('0'));
    }

    qr_code_rows_.append(row_data);
  }

  QRcode_free(qr_code);
}

void FileShareTrayController::AttachServiceListeners() {
  NearbySharingApi::Listener listener;

  listener.target_discovered_cb = [this](const NearbySharingApi::ShareTargetInfo& info) {
    QMetaObject::invokeMethod(
        this,
        [this, info]() {
          const QString name = TrimmedOrFallback(
              QString::fromStdString(info.device_name),
              QStringLiteral("Unknown device"));
          UpsertTarget(info.id, name, info.is_incoming);
        },
        Qt::QueuedConnection);
  };

  listener.target_updated_cb = [this](const NearbySharingApi::ShareTargetInfo& info) {
    QMetaObject::invokeMethod(
        this,
        [this, info]() {
          const QString name = TrimmedOrFallback(
              QString::fromStdString(info.device_name),
              QStringLiteral("Unknown device"));
          UpsertTarget(info.id, name, info.is_incoming);
        },
        Qt::QueuedConnection);
  };

  listener.target_lost_cb = [this](int64_t share_target_id) {
    QMetaObject::invokeMethod(
        this,
        [this, share_target_id]() {
          RemoveTarget(share_target_id);
        },
        Qt::QueuedConnection);
  };

  listener.transfer_update_cb =
      [this](const NearbySharingApi::TransferUpdateInfo& update) {
        QMetaObject::invokeMethod(
            this,
            [this, update]() {
              const QString update_name =
                  QString::fromStdString(update.device_name).trimmed();
              if (!update_name.isEmpty()) {
                target_names_[update.share_target_id] = update_name;
              }
              const QString name = TargetName(update.share_target_id);
              const QString status = TransferStatusToString(update.status);
              const QString direction =
                  update.is_incoming ? QStringLiteral("incoming")
                                     : QStringLiteral("outgoing");
              QString file_name = QString::fromStdString(update.first_file_name);
              if (file_name.isEmpty() && !update.is_incoming &&
                  pending_send_target_id_ == update.share_target_id &&
                  !pending_send_file_name_.isEmpty()) {
                file_name = pending_send_file_name_;
              }

              UpsertTransfer(update.share_target_id, name, status, update.progress,
                             update.transferred_bytes, direction, file_name);
              SetStatus(QStringLiteral("%1 (%2)")
                            .arg(status)
                            .arg(name));

              if (update.status ==
                  NearbySharingApi::TransferStatus::kAwaitingLocalConfirmation) {
                if (auto_accept_incoming_) {
                  service_->Accept(
                      update.share_target_id, [](NearbySharingApi::StatusCode) {});
                }
              }

              if (!IsFinalTransferStatus(update.status)) {
                return;
              }

              const bool success =
                  update.status == NearbySharingApi::TransferStatus::kComplete;

              if (update.is_incoming) {
                if (success) {
                  const QString received_link = FirstReceivedUrl(update);
                  if (!received_link.isEmpty()) {
                    emit requestCopyLinkTrayMessage(
                        QStringLiteral("Link received"),
                        QStringLiteral("%1 from %2")
                            .arg(received_link, name),
                        received_link);
                  } else if (!update.text_attachments.empty()) {
                    const QString received_text =
                        TrimmedOrFallback(FirstReceivedTextSummary(update),
                                          QStringLiteral("Text"));
                    emit requestTrayMessage(
                        QStringLiteral("Text received"),
                        QStringLiteral("%1 from %2")
                            .arg(received_text, name));
                  } else {
                    const QString received_name =
                        file_name.isEmpty() ? QStringLiteral("file")
                                            : file_name;
                    emit requestTrayMessage(
                        QStringLiteral("File received"),
                        QStringLiteral("%1 from %2").arg(received_name, name));
                  }
                } else {
                  emit requestTrayMessage(
                      QStringLiteral("Receive failed"),
                      QStringLiteral("Transfer from %1 failed (%2)")
                          .arg(name, status));
                }
                return;
              }

              const QString sent_name = file_name.isEmpty()
                                            ? QStringLiteral("file")
                                            : file_name;
              if (success) {
                emit requestTrayMessage(QStringLiteral("Send complete"),
                                        QStringLiteral("%1 sent to %2")
                                            .arg(sent_name, name));
              } else {
                emit requestTrayMessage(
                    QStringLiteral("Send failed"),
                    QStringLiteral("%1 failed to send to %2")
                        .arg(sent_name, name));
              }

              if (pending_send_target_id_ == update.share_target_id) {
                pending_send_target_id_ = 0;
                if (!pending_send_file_path_.isEmpty()) {
                  pending_send_file_path_.clear();
                  emit pendingSendFilePathChanged();
                }
                if (!pending_send_file_name_.isEmpty()) {
                  pending_send_file_name_.clear();
                  emit pendingSendFileNameChanged();
                }
              }

              if (pending_target_removals_.contains(update.share_target_id)) {
                QTimer::singleShot(1400, this, [this, target_id = update.share_target_id]() {
                  if (!pending_target_removals_.contains(target_id)) {
                    return;
                  }
                  pending_target_removals_.remove(target_id);
                  RemoveTarget(target_id);
                });
              }
            },
            Qt::QueuedConnection);
      };

  service_->SetListener(std::move(listener));
}

void FileShareTrayController::LoadSettings() {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlFileTrayApp"));

  const QString stored_device_name =
      settings.value(QStringLiteral("deviceName"), device_name_)
          .toString()
          .trimmed();
  if (!stored_device_name.isEmpty()) {
    device_name_ = stored_device_name;
  }

  auto_accept_incoming_ =
      settings.value(QStringLiteral("autoAcceptIncoming"), auto_accept_incoming_)
          .toBool();

  const QString stored_log_path =
      settings.value(QStringLiteral("logPath"), log_path_).toString().trimmed();
  if (!stored_log_path.isEmpty()) {
    log_path_ = stored_log_path;
  }
}

void FileShareTrayController::SaveSettings() const {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlFileTrayApp"));
  settings.setValue(QStringLiteral("deviceName"), device_name_);
  settings.setValue(QStringLiteral("autoAcceptIncoming"), auto_accept_incoming_);
  settings.setValue(QStringLiteral("logPath"), log_path_);
  settings.sync();
}

void FileShareTrayController::setDeviceName(const QString& device_name) {
  const QString trimmed = device_name.trimmed();
  if (trimmed.isEmpty() || trimmed == device_name_) {
    return;
  }

  const bool was_running = running_;
  if (was_running) {
    stop();
  }

  device_name_ = trimmed;
  emit deviceNameChanged();
  SaveSettings();
  if (service_) {
    //service_->SetDeviceName(device_name_.toStdString());
    //qr_code_url_ = QString::fromStdString(service_->GetQrCodeUrl());
    //UpdateQrCodeData();
    //emit qrCodeUrlChanged();
    //emit qrCodeChanged();
    service_->Shutdown([](NearbySharingApi::StatusCode) {});
    service_.reset();
  }
  CreateService();

  if (was_running) {
    start();
  }
}

void FileShareTrayController::setAutoAcceptIncoming(bool enabled) {
  if (auto_accept_incoming_ == enabled) return;
  auto_accept_incoming_ = enabled;
  emit autoAcceptIncomingChanged();
  SaveSettings();
}

void FileShareTrayController::setLogPath(const QString& path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty() || trimmed == log_path_) return;
  log_path_ = trimmed;
  emit logPathChanged();
  SaveSettings();
}

void FileShareTrayController::start() {
  if (running_) {
    return;
  }

  running_ = true;
  emit runningChanged();

  if (mode_ == QStringLiteral("Send")) {
    startSendMode();
  } else {
    startReceiveMode();
  }
}

void FileShareTrayController::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  emit runningChanged();

  service_->StopSendMode([](NearbySharingApi::StatusCode) {});
  service_->StopReceiveMode([](NearbySharingApi::StatusCode) {});

  discovered_targets_.clear();
  discovered_row_by_target_.clear();
  pending_target_removals_.clear();
  target_names_.clear();
  transfers_.clear();
  transfer_row_by_target_.clear();
  pending_send_target_id_ = 0;

  emit discoveredTargetsChanged();
  emit transfersChanged();

  SetStatus(QStringLiteral("Stopped"));
}

void FileShareTrayController::switchToReceiveMode() {
  if (running_ && HasActiveTransfers()) {
    SetStatus(QStringLiteral("Cannot switch mode while transfer is active"));
    emit requestTrayMessage(
        QStringLiteral("Transfer in progress"),
        QStringLiteral("Wait for the current transfer to complete."));
    return;
  }

  if (mode_ != QStringLiteral("Receive")) {
    mode_ = QStringLiteral("Receive");
    emit modeChanged();
    if (running_) {
      stop();
      start();
      return;
    }
  }

  if (!running_) {
    start();
  }
}

void FileShareTrayController::switchToSendModeWithFile(const QString& file_path) {
  const QString trimmed_path = file_path.trimmed();
  QFileInfo info(trimmed_path);
  if (trimmed_path.isEmpty() || !info.exists() || !info.isFile()) {
    SetStatus(QStringLiteral("Selected file is not valid"));
    emit requestTrayMessage(QStringLiteral("Send canceled"),
                            QStringLiteral("Please choose a valid file."));
    return;
  }

  pending_send_file_path_ = info.absoluteFilePath();
  pending_send_file_name_ = info.fileName();
  emit pendingSendFilePathChanged();
  emit pendingSendFileNameChanged();

  if (running_ && HasActiveTransfers()) {
    SetStatus(QStringLiteral("Cannot switch mode while transfer is active"));
    emit requestTrayMessage(
        QStringLiteral("Transfer in progress"),
        QStringLiteral("Wait for the current transfer to complete."));
    return;
  }

  if (mode_ != QStringLiteral("Send")) {
    mode_ = QStringLiteral("Send");
    emit modeChanged();
    if (running_) {
      stop();
      start();
    }
  }

  if (!running_) {
    start();
  }

  SetStatus(QStringLiteral("Discovery started. Choose a nearby device."));
  emit requestTrayMessage(
      QStringLiteral("Send mode"),
      QStringLiteral("Selected %1. Choose a nearby device to send.")
          .arg(pending_send_file_name_));
}

void FileShareTrayController::sendPendingFileToTarget(qlonglong share_target_id) {
  if (share_target_id <= 0) {
    return;
  }

  QFileInfo file_info(pending_send_file_path_);
  if (pending_send_file_path_.isEmpty() || !file_info.exists() ||
      !file_info.isFile()) {
    SetStatus(QStringLiteral("Selected file is not available"));
    emit requestTrayMessage(QStringLiteral("Send failed"),
                            QStringLiteral("Selected file is not available."));
    return;
  }

  const QString target_name = TargetName(share_target_id);
  pending_send_target_id_ = share_target_id;
  UpsertTransfer(share_target_id, target_name, QStringLiteral("Queued"), 0.0, 0,
                 QStringLiteral("outgoing"), pending_send_file_name_);

  service_->SendFile(
      share_target_id, file_info.absoluteFilePath().toStdString(),
      [this, share_target_id](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, share_target_id, status]() {
              const QString target_name = TargetName(share_target_id);
              if (status == NearbySharingApi::StatusCode::kOk) {
                SetStatus(QStringLiteral("Sending %1 to %2")
                              .arg(pending_send_file_name_, target_name));
                return;
              }

              emit requestTrayMessage(
                  QStringLiteral("Send failed"),
                  QStringLiteral("Could not send to %1").arg(target_name));
              UpsertTransfer(share_target_id, target_name, QStringLiteral("Failed"),
                             0.0, 0, QStringLiteral("outgoing"),
                             pending_send_file_name_);
              pending_send_target_id_ = 0;
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::copyTextToClipboard(const QString& text) {
  const QString trimmed = text.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }

  QClipboard* clipboard = QGuiApplication::clipboard();
  if (clipboard == nullptr) {
    emit requestTrayMessage(QStringLiteral("Copy failed"),
                            QStringLiteral("Clipboard is not available."));
    return;
  }

  clipboard->setText(trimmed, QClipboard::Clipboard);
  SetStatus(QStringLiteral("Connection URL copied to clipboard"));
  emit requestTrayMessage(QStringLiteral("URL copied"),
                          QStringLiteral("Connection URL copied to clipboard."));
}

void FileShareTrayController::clearTransfers() {
  transfers_.clear();
  transfer_row_by_target_.clear();
  emit transfersChanged();
}

void FileShareTrayController::hideToTray() {
  emit requestTrayMessage(
      QStringLiteral("Nearby File Tray"),
      QStringLiteral("App is still running in the system tray."));
}

void FileShareTrayController::startSendMode() {
  service_->StopReceiveMode([this](NearbySharingApi::StatusCode status)
  {
    if (status == NearbySharingApi::StatusCode::kOk || status == NearbySharingApi::StatusCode::kStatusAlreadyStopped)
    {
      service_->StartSendMode([this](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              SetStatus(QStringLiteral("StartSendMode: %1").arg(StatusToString(status)));
              if (status != NearbySharingApi::StatusCode::kOk) {
                running_ = false;
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
      }

  });
}

void FileShareTrayController::startReceiveMode() {
  service_->StopSendMode([this](NearbySharingApi::StatusCode status)
  {
    if (status == NearbySharingApi::StatusCode::kOk || status == NearbySharingApi::StatusCode::kStatusAlreadyStopped)
    {

      service_->StartReceiveMode([this](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              SetStatus(
                  QStringLiteral("StartReceiveMode: %1").arg(StatusToString(status)));
              if (status != NearbySharingApi::StatusCode::kOk) {
                running_ = false;
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
      }

  });
}

void FileShareTrayController::UpsertTarget(qlonglong share_target_id,
                                           const QString& name,
                                           bool is_incoming) {
  pending_target_removals_.remove(share_target_id);
  target_names_[share_target_id] = name;
  if (is_incoming) {
    return;
  }

  QVariantMap row{
      {QStringLiteral("id"), share_target_id},
      {QStringLiteral("name"), name},
  };

  if (discovered_row_by_target_.contains(share_target_id)) {
    const int row_index = discovered_row_by_target_.value(share_target_id);
    if (row_index >= 0 && row_index < discovered_targets_.size()) {
      discovered_targets_[row_index] = row;
      emit discoveredTargetsChanged();
      return;
    }
  }

  discovered_row_by_target_.insert(share_target_id, discovered_targets_.size());
  discovered_targets_.append(row);
  emit discoveredTargetsChanged();
}

void FileShareTrayController::RemoveTarget(qlonglong share_target_id) {
  if (HasActiveTransferForTarget(share_target_id)) {
    pending_target_removals_.insert(share_target_id);
    return;
  }

  pending_target_removals_.remove(share_target_id);
  target_names_.remove(share_target_id);
  if (!discovered_row_by_target_.contains(share_target_id)) {
    return;
  }

  const int removed_index = discovered_row_by_target_.take(share_target_id);
  if (removed_index < 0 || removed_index >= discovered_targets_.size()) {
    return;
  }

  discovered_targets_.removeAt(removed_index);
  for (auto it = discovered_row_by_target_.begin();
       it != discovered_row_by_target_.end(); ++it) {
    if (it.value() > removed_index) {
      it.value() = it.value() - 1;
    }
  }
  emit discoveredTargetsChanged();
}

QString FileShareTrayController::TargetName(qlonglong share_target_id) const {
  const QString name = target_names_.value(share_target_id).trimmed();
  return name.isEmpty() ? QStringLiteral("Unknown device") : name;
}

bool FileShareTrayController::HasActiveTransferForTarget(
    qlonglong share_target_id) const {
  for (const QVariant& row_value : transfers_) {
    const QVariantMap row = row_value.toMap();
    if (row.value(QStringLiteral("targetId")).toLongLong() != share_target_id) {
      continue;
    }

    if (IsActiveTransferStatus(row.value(QStringLiteral("status")).toString())) {
      return true;
    }
  }
  return false;
}

void FileShareTrayController::UpsertTransfer(
    qlonglong share_target_id, const QString& target_name, const QString& status,
    double progress, qulonglong transferred_bytes, const QString& direction,
    const QString& file_name) {
  QVariantMap transfer{
      {QStringLiteral("targetId"), share_target_id},
      {QStringLiteral("targetName"), target_name},
      {QStringLiteral("status"), status},
      {QStringLiteral("progress"), progress},
      {QStringLiteral("transferredBytes"), transferred_bytes},
      {QStringLiteral("direction"), direction},
      {QStringLiteral("fileName"), file_name},
  };

  if (transfer_row_by_target_.contains(share_target_id)) {
    const int row_index = transfer_row_by_target_.value(share_target_id);
    if (row_index >= 0 && row_index < transfers_.size()) {
      transfers_[row_index] = transfer;
      emit transfersChanged();
      return;
    }
  }

  transfer_row_by_target_.insert(share_target_id, transfers_.size());
  transfers_.append(transfer);
  emit transfersChanged();
}

void FileShareTrayController::SetStatus(const QString& status) {
  if (status == status_message_) {
    return;
  }
  status_message_ = status;
  emit statusMessageChanged();
}

bool FileShareTrayController::HasActiveTransfers() const {
  for (const QVariant& row_value : transfers_) {
    const QVariantMap row = row_value.toMap();
    const QString status = row.value(QStringLiteral("status")).toString();
    if (IsActiveTransferStatus(status)) {
      return true;
    }
  }
  return false;
}

bool FileShareTrayController::IsActiveTransferStatus(const QString& status) {
  return status == QStringLiteral("Queued") ||
         status == QStringLiteral("Connecting") ||
         status == QStringLiteral("AwaitingLocalConfirmation") ||
         status == QStringLiteral("AwaitingRemoteAcceptance") ||
         status == QStringLiteral("InProgress");
}

QString FileShareTrayController::StatusToString(NearbySharingApi::StatusCode status) {
  return QString::fromStdString(NearbySharingApi::StatusCodeToString(status));
}

QString FileShareTrayController::TransferStatusToString(
    NearbySharingApi::TransferStatus status) {
  return QString::fromStdString(NearbySharingApi::TransferStatusToString(status));
}

bool FileShareTrayController::IsFinalTransferStatus(
    NearbySharingApi::TransferStatus status) {
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
