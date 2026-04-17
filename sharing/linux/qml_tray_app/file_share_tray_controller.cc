#include "file_share_tray_controller.h"

#include <iostream>
#include <QClipboard>
#include <QDesktopServices>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>

#include "string_utils.h"
#include "status_mapper.h"
#include "qr_code_generator.h"

FileShareTrayController::FileShareTrayController(QObject* parent)
    : QObject(parent) {
  const QString host = QSysInfo::machineHostName().trimmed();
  if (!host.isEmpty()) {
    state_.SetDeviceName(host);
  }

  loadSettings();
  initializeService();
}

FileShareTrayController::~FileShareTrayController() {
  stop();
  if (service_) {
    service_->Shutdown([](NearbySharingApi::StatusCode) {});
  }
}

void FileShareTrayController::initializeService() {
  service_ = std::make_unique<NearbySharingApi>(state_.deviceName().toStdString());
  service_->Set5GhzHotspotEnabled(state_.enable5GhzHotspot());
  state_.SetQrCodeData(QString::fromStdString(service_->GetQrCodeUrl()), {}, 0);
  updateQrCodeData();
  emit qrCodeUrlChanged();
  emit qrCodeChanged();
  attachServiceListeners();
  // service_ ->StartFastInitiationScanning([](auto a)
  // {
  //   std::cout << "Probably fine";
  // });
}

void FileShareTrayController::updateQrCodeData() {
  const auto qr_data = QrCodeGenerator::GenerateQrCode(state_.qrCodeUrl());
  state_.SetQrCodeData(state_.qrCodeUrl(), qr_data.rows, qr_data.size);
  emit qrCodeChanged();
}

void FileShareTrayController::attachServiceListeners() {
  NearbySharingApi::Listener listener;

  listener.target_discovered_cb = [this](const NearbySharingApi::ShareTargetInfo& info) {
    QMetaObject::invokeMethod(this, [this, info]() { updateTargetFromInfo(info); },
                              Qt::QueuedConnection);
  };

  listener.target_updated_cb = [this](const NearbySharingApi::ShareTargetInfo& info) {
    QMetaObject::invokeMethod(this, [this, info]() { updateTargetFromInfo(info); },
                              Qt::QueuedConnection);
  };

  listener.target_lost_cb = [this](int64_t share_target_id) {
    QMetaObject::invokeMethod(
        this, [this, share_target_id]() {
          state_.RemoveTarget(share_target_id);
          emit discoveredTargetsChanged();
        },
        Qt::QueuedConnection);
  };

  listener.transfer_update_cb = [this](const NearbySharingApi::TransferUpdateInfo& update) {
    QMetaObject::invokeMethod(this, [this, update]() { handleTransferUpdate(update); },
                              Qt::QueuedConnection);
  };

  service_->SetListener(std::move(listener));
}

void FileShareTrayController::updateTargetFromInfo(
    const NearbySharingApi::ShareTargetInfo& info) {
  const QString name = StringUtils::TrimmedOrFallback(
      StringUtils::FromStdString(info.device_name),
      QStringLiteral("Unknown device"));
  state_.AddOrUpdateTarget(info.id, name, info.is_incoming);
  emit discoveredTargetsChanged();
}

void FileShareTrayController::handleTransferUpdate(
    const NearbySharingApi::TransferUpdateInfo& update) {
  const QString target_name = StringUtils::TrimmedFromStdString(update.device_name);
  if (!target_name.isEmpty()) {
    state_.AddOrUpdateTarget(update.share_target_id, target_name, update.is_incoming);
  }

  const QString name = state_.GetTargetName(update.share_target_id);
  const QString status = StatusMapper::TransferStatusToString(update.status);
  const QString direction =
      update.is_incoming ? QStringLiteral("incoming") : QStringLiteral("outgoing");

  QString file_name = StringUtils::FromStdString(update.first_file_name);
  if (file_name.isEmpty() && !update.is_incoming &&
      state_.pendingSendTargetId() == update.share_target_id &&
      !state_.pendingSendFileName().isEmpty()) {
    file_name = state_.pendingSendFileName();
  }

  state_.AddOrUpdateTransfer(update.share_target_id, name, status, update.progress,
                             update.transferred_bytes, direction, file_name,
                             StringUtils::FromStdString(update.first_file_path));
  emit transfersChanged();

  setStatus(QStringLiteral("%1 (%2)").arg(status, name));

  // Auto-accept incoming transfers if enabled
  if (update.status == NearbySharingApi::TransferStatus::kAwaitingLocalConfirmation &&
      state_.autoAcceptIncoming()) {
    service_->Accept(update.share_target_id, [](NearbySharingApi::StatusCode) {});
  }

  // Handle final transfer status
  if (StatusMapper::IsFinalTransferStatus(update.status)) {
    handleTransferComplete(update);
  }
}

void FileShareTrayController::handleTransferComplete(
    const NearbySharingApi::TransferUpdateInfo& update) {
  const bool success = update.status == NearbySharingApi::TransferStatus::kComplete;
  const QString name = state_.GetTargetName(update.share_target_id);

  if (update.is_incoming) {
    handleIncomingTransferComplete(update, name, success);
  } else {
    handleOutgoingTransferComplete(update, name, success);
  }

  // Cleanup pending send state
  if (state_.pendingSendTargetId() == update.share_target_id) {
    state_.ClearPendingSendFile();
    emit pendingSendFilePathChanged();
    emit pendingSendFileNameChanged();

    // Auto-switch to receive mode after successful send
    if (!update.is_incoming && success) {
      switchToReceiveMode();
    }
  }

  // Deferred target removal
  if (state_.IsPendingTargetRemoval(update.share_target_id)) {
    QTimer::singleShot(1400, this, [this, target_id = update.share_target_id]() {
      if (state_.IsPendingTargetRemoval(target_id)) {
        state_.RemoveTarget(target_id);
        emit discoveredTargetsChanged();
      }
    });
  }
}

void FileShareTrayController::handleIncomingTransferComplete(
    const NearbySharingApi::TransferUpdateInfo& update, const QString& name,
    bool success) {
  if (!success) {
    emit requestTrayMessage(
        QStringLiteral("Receive failed"),
        QStringLiteral("Transfer from %1 failed").arg(name));
    return;
  }

  const QString file_name =
      StringUtils::FromStdString(update.first_file_name).isEmpty()
          ? QStringLiteral("file")
          : StringUtils::FromStdString(update.first_file_name);

  // Check for received URL
  for (const auto& text : update.text_attachments) {
    if (text.type == NearbySharingApi::TextAttachmentType::kUrl) {
      const QString link = StringUtils::TrimmedFromStdString(text.text_body);
      if (!link.isEmpty()) {
        emit requestCopyLinkTrayMessage(QStringLiteral("Link received"),
                                        QStringLiteral("%1 from %2").arg(link, name),
                                        link);
        return;
      }
    }
  }

  // Check for received text
  if (!update.text_attachments.empty()) {
    const QString text_summary = [&]() {
      for (const auto& text : update.text_attachments) {
        const QString title = StringUtils::TrimmedFromStdString(text.text_title);
        if (!title.isEmpty()) return title;
        const QString body = StringUtils::TrimmedFromStdString(text.text_body);
        if (!body.isEmpty()) return body;
      }
      return QStringLiteral("Text");
    }();
    emit requestTrayMessage(QStringLiteral("Text received"),
                            QStringLiteral("%1 from %2").arg(text_summary, name));
    return;
  }

  emit requestTrayMessage(QStringLiteral("File received"),
                          QStringLiteral("%1 from %2").arg(file_name, name));
}

void FileShareTrayController::handleOutgoingTransferComplete(
    const NearbySharingApi::TransferUpdateInfo& update, const QString& name,
    bool success) {
  const QString file_name =
      StringUtils::FromStdString(update.first_file_name).isEmpty()
          ? QStringLiteral("file")
          : StringUtils::FromStdString(update.first_file_name);

  if (success) {
    emit requestTrayMessage(QStringLiteral("Send complete"),
                            QStringLiteral("%1 sent to %2").arg(file_name, name));
  } else {
    emit requestTrayMessage(QStringLiteral("Send failed"),
                            QStringLiteral("%1 failed to send to %2").arg(file_name, name));
  }
}

void FileShareTrayController::loadSettings() {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlFileTrayApp"));

  const QString stored_device_name =
      settings.value(QStringLiteral("deviceName"), state_.deviceName())
          .toString()
          .trimmed();
  if (!stored_device_name.isEmpty()) {
    state_.SetDeviceName(stored_device_name);
  }

  const bool stored_auto_accept =
      settings.value(QStringLiteral("autoAcceptIncoming"), true).toBool();
  state_.SetAutoAcceptIncoming(stored_auto_accept);

  const bool stored_enable_5ghz_hotspot =
      settings.value(QStringLiteral("enable5GhzHotspot"), true).toBool();
  state_.SetEnable5GhzHotspot(stored_enable_5ghz_hotspot);

  const QString stored_log_path =
      settings.value(QStringLiteral("logPath"), QStringLiteral("/tmp/nearby_qml_file_tray.log"))
          .toString()
          .trimmed();
  if (!stored_log_path.isEmpty()) {
    state_.SetLogPath(stored_log_path);
  }
}

void FileShareTrayController::saveSettings() const {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlFileTrayApp"));
  settings.setValue(QStringLiteral("deviceName"), state_.deviceName());
  settings.setValue(QStringLiteral("autoAcceptIncoming"), state_.autoAcceptIncoming());
  settings.setValue(QStringLiteral("enable5GhzHotspot"),
                    state_.enable5GhzHotspot());
  settings.setValue(QStringLiteral("logPath"), state_.logPath());
}

void FileShareTrayController::setDeviceName(const QString& device_name) {
  const QString trimmed = device_name.trimmed();
  if (trimmed.isEmpty() || trimmed == state_.deviceName()) {
    return;
  }

  state_.SetDeviceName(trimmed);
  saveSettings();
  emit deviceNameChanged();

  if (state_.running()) {
    stop();
    initializeService();
    start();
  }
}

void FileShareTrayController::setAutoAcceptIncoming(bool enabled) {
  if (enabled == state_.autoAcceptIncoming()) {
    return;
  }
  state_.SetAutoAcceptIncoming(enabled);
  saveSettings();
  emit autoAcceptIncomingChanged();
}

void FileShareTrayController::setEnable5GhzHotspot(bool enabled) {
  if (enabled == state_.enable5GhzHotspot()) {
    return;
  }
  state_.SetEnable5GhzHotspot(enabled);
  if (service_) {
    service_->Set5GhzHotspotEnabled(enabled);
  }
  saveSettings();
  emit enable5GhzHotspotChanged();
}

void FileShareTrayController::setLogPath(const QString& path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty() || trimmed == state_.logPath()) {
    return;
  }
  state_.SetLogPath(trimmed);
  saveSettings();
  emit logPathChanged();
}

void FileShareTrayController::start() {
  if (state_.running()) {
    return;
  }

  state_.SetRunning(true);
  emit runningChanged();

}

void FileShareTrayController::stop() {
  if (!state_.running()) {
    return;
  }

  state_.SetRunning(false);
  emit runningChanged();

  service_->StopSendMode([](NearbySharingApi::StatusCode) {});
  service_->StopReceiveMode([](NearbySharingApi::StatusCode) {});

  state_.ClearAll();
  emit discoveredTargetsChanged();
  emit transfersChanged();

  setStatus(QStringLiteral("Stopped"));
}

void FileShareTrayController::startSendMode() {
  service_->StopReceiveMode([this](NearbySharingApi::StatusCode status) {
    if (status == NearbySharingApi::StatusCode::kOk ||
        status == NearbySharingApi::StatusCode::kStatusAlreadyStopped) {
      service_->StartSendMode([this](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              setStatus(QStringLiteral("StartSendMode: %1")
                            .arg(StatusMapper::ApiStatusToString(status)));
              if (status != NearbySharingApi::StatusCode::kOk) {
                state_.SetRunning(false);
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
    }
  });
}

void FileShareTrayController::startReceiveMode() {
  service_->StopSendMode([this](NearbySharingApi::StatusCode status) {
    if (status == NearbySharingApi::StatusCode::kOk ||
        status == NearbySharingApi::StatusCode::kStatusAlreadyStopped) {
      service_->StartReceiveMode([this](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              setStatus(QStringLiteral("StartReceiveMode: %1")
                            .arg(StatusMapper::ApiStatusToString(status)));
              if (status != NearbySharingApi::StatusCode::kOk) {
                state_.SetRunning(false);
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
    }
  });
}

void FileShareTrayController::switchToReceiveMode() {
  if (state_.running() && state_.HasActiveTransfers()) {
    setStatus(QStringLiteral("Cannot switch mode while transfer is active"));
    emit requestTrayMessage(
        QStringLiteral("Transfer in progress"),
        QStringLiteral("Wait for the current transfer to complete."));
    return;
  }
  if (state_.running())
  {
    startReceiveMode();
    state_.SetMode(QStringLiteral("Receive"));
    emit modeChanged();
  }
}

void FileShareTrayController::switchToSendModeWithFile(const QString& file_path) {
  const QString trimmed_path = file_path.trimmed();
  QFileInfo info(trimmed_path);

  if (trimmed_path.isEmpty() || !info.exists() || !info.isFile()) {
    setStatus(QStringLiteral("Selected file is not valid"));
    emit requestTrayMessage(QStringLiteral("Send canceled"),
                            QStringLiteral("Please choose a valid file."));
    return;
  }

  state_.SetPendingSendFile(info.absoluteFilePath(), info.fileName(), 0);
  emit pendingSendFilePathChanged();
  emit pendingSendFileNameChanged();

  if (state_.running() && state_.HasActiveTransfers()) {
    setStatus(QStringLiteral("Cannot switch mode while transfer is active"));
    emit requestTrayMessage(
        QStringLiteral("Transfer in progress"),
        QStringLiteral("Wait for the current transfer to complete."));
    return;
  }
  if (state_.running())
  {
    startSendMode();
    state_.SetMode(QStringLiteral("Send"));
    emit modeChanged();
  }

  setStatus(QStringLiteral("Discovery started. Choose a nearby device."));
  emit requestTrayMessage(
      QStringLiteral("Send mode"),
      QStringLiteral("Selected %1. Choose a nearby device to send.")
          .arg(info.fileName()));
}

void FileShareTrayController::sendPendingFileToTarget(qlonglong share_target_id) {
  if (share_target_id <= 0) {
    return;
  }

  const QString file_path = state_.pendingSendFilePath();
  QFileInfo file_info(file_path);

  if (file_path.isEmpty() || !file_info.exists() || !file_info.isFile()) {
    setStatus(QStringLiteral("Selected file is not available"));
    emit requestTrayMessage(QStringLiteral("Send failed"),
                            QStringLiteral("Selected file is not available."));
    return;
  }

  const QString target_name = state_.GetTargetName(share_target_id);
  state_.SetPendingSendFile(file_path, file_info.fileName(), share_target_id);

  state_.AddOrUpdateTransfer(share_target_id, target_name, QStringLiteral("Queued"), 0.0, 0,
                             QStringLiteral("outgoing"), file_info.fileName(),
                             file_info.absoluteFilePath());
  emit transfersChanged();

  service_->SendFile(
      share_target_id, file_info.absoluteFilePath().toStdString(),
      [this, share_target_id](NearbySharingApi::StatusCode status) {
        QMetaObject::invokeMethod(
            this,
            [this, share_target_id, status]() {
              const QString target_name = state_.GetTargetName(share_target_id);
              if (status == NearbySharingApi::StatusCode::kOk) {
                setStatus(QStringLiteral("Sending %1 to %2")
                              .arg(state_.pendingSendFileName(), target_name));
                return;
              }

              emit requestTrayMessage(
                  QStringLiteral("Send failed"),
                  QStringLiteral("Could not send to %1").arg(target_name));

              state_.AddOrUpdateTransfer(share_target_id, target_name,
                                         QStringLiteral("Failed"), 0.0, 0,
                                         QStringLiteral("outgoing"),
                                         state_.pendingSendFileName(),
                                         state_.pendingSendFilePath());
              emit transfersChanged();
              state_.SetPendingSendFile("", "", 0);
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
  setStatus(QStringLiteral("Connection URL copied to clipboard"));
  emit requestTrayMessage(QStringLiteral("URL copied"),
                          QStringLiteral("Link copied to clipboard."));
}

void FileShareTrayController::openFileLocation(const QString& file_path) {
  const QString trimmed = file_path.trimmed();
  if (trimmed.isEmpty()) {
    emit requestTrayMessage(QStringLiteral("Open location failed"),
                            QStringLiteral("No received file location is available."));
    return;
  }

  QFileInfo info(trimmed);
  QString target_path;
  if (info.exists() && info.isFile()) {
    // Open the containing folder so the file is visible in the user's file
    // manager regardless of the desktop environment.
    target_path = info.absolutePath();
  } else if (info.exists() && info.isDir()) {
    target_path = info.absoluteFilePath();
  } else {
    // Some transfer updates can outlive the exact file entry we saw earlier;
    // fall back to the parent directory when it still exists.
    const QFileInfo parent_info(info.absolutePath());
    if (parent_info.exists() && parent_info.isDir()) {
      target_path = parent_info.absoluteFilePath();
    }
  }

  if (target_path.isEmpty()) {
    emit requestTrayMessage(QStringLiteral("Open location failed"),
                            QStringLiteral("The file location is no longer available."));
    return;
  }

  const bool opened =
      QDesktopServices::openUrl(QUrl::fromLocalFile(target_path));
  if (!opened) {
    emit requestTrayMessage(QStringLiteral("Open location failed"),
                            QStringLiteral("Could not open the file location."));
  }
}

void FileShareTrayController::clearTransfers() {
  state_.ClearAll();
  emit discoveredTargetsChanged();
  emit transfersChanged();
}

void FileShareTrayController::hideToTray() {
  // This is handled by the main window, but can be extended here if needed
}

void FileShareTrayController::setStatus(const QString& status) {
  if (status == state_.statusMessage()) {
    return;
  }
  state_.SetStatusMessage(status);
  emit statusMessageChanged();
}

void FileShareTrayController::notifyStateChange(const QString& property) {
  if (property == QStringLiteral("mode")) {
    emit modeChanged();
  } else if (property == QStringLiteral("deviceName")) {
    emit deviceNameChanged();
  } else if (property == QStringLiteral("statusMessage")) {
    emit statusMessageChanged();
  } else if (property == QStringLiteral("running")) {
    emit runningChanged();
  } else if (property == QStringLiteral("autoAcceptIncoming")) {
    emit autoAcceptIncomingChanged();
  } else if (property == QStringLiteral("enable5GhzHotspot")) {
    emit enable5GhzHotspotChanged();
  } else if (property == QStringLiteral("discoveredTargets")) {
    emit discoveredTargetsChanged();
  } else if (property == QStringLiteral("transfers")) {
    emit transfersChanged();
  }
}
