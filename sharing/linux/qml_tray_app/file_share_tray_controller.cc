#include "file_share_tray_controller.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QSettings>
#include <QSysInfo>
#include <QTextStream>

#include <atomic>
#include <memory>

namespace {

using NearbyConnectionsQtFacade = nearby::sharing::NearbyConnectionsQtFacade;

std::atomic<qlonglong> g_local_payload_id{1'000'000};

bool IsTerminalPayloadStatus(NearbyConnectionsQtFacade::PayloadStatus status) {
  return status == NearbyConnectionsQtFacade::PayloadStatus::kSuccess ||
         status == NearbyConnectionsQtFacade::PayloadStatus::kFailure ||
         status == NearbyConnectionsQtFacade::PayloadStatus::kCanceled;
}

QString NormalizeConnectionStrategy(const QString& strategy) {
  const QString token = strategy.trimmed().toLower();
  if (token == QStringLiteral("p2pstar") || token == QStringLiteral("star")) {
    return QStringLiteral("P2pStar");
  }
  if (token == QStringLiteral("p2ppointtopoint") ||
      token == QStringLiteral("pointtopoint") ||
      token == QStringLiteral("point_to_point")) {
    return QStringLiteral("P2pPointToPoint");
  }
  return QStringLiteral("P2pCluster");
}

NearbyConnectionsQtFacade::Strategy StrategyFromName(
    const QString& normalized_strategy) {
  if (normalized_strategy == QStringLiteral("P2pStar")) {
    return NearbyConnectionsQtFacade::Strategy::kP2pStar;
  }
  if (normalized_strategy == QStringLiteral("P2pPointToPoint")) {
    return NearbyConnectionsQtFacade::Strategy::kP2pPointToPoint;
  }
  return NearbyConnectionsQtFacade::Strategy::kP2pCluster;
}

}  // namespace

FileShareTrayController::FileShareTrayController(QObject* parent)
    : QObject(parent) {
  const QString host = QSysInfo::machineHostName();
  if (!host.isEmpty()) {
    device_name_ = host;
  }

  LoadSettings();
  ReopenLogFile();
  LogLine(QStringLiteral("Started file share tray controller"));
}

FileShareTrayController::~FileShareTrayController() {
  stop();
  if (log_file_.isOpen()) {
    log_file_.close();
  }
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
  bluetooth_enabled_ =
      settings.value(QStringLiteral("bluetoothEnabled"), bluetooth_enabled_).toBool();
  ble_enabled_ =
      settings.value(QStringLiteral("bleEnabled"), ble_enabled_).toBool();
  wifi_lan_enabled_ =
      settings.value(QStringLiteral("wifiLanEnabled"), wifi_lan_enabled_).toBool();
  wifi_hotspot_enabled_ =
      settings.value(QStringLiteral("wifiHotspotEnabled"), wifi_hotspot_enabled_).toBool();
  web_rtc_enabled_ =
      settings.value(QStringLiteral("webRtcEnabled"), web_rtc_enabled_).toBool();
  connection_strategy_ = NormalizeConnectionStrategy(
      settings.value(QStringLiteral("connectionStrategy"), connection_strategy_)
          .toString());

  const QString stored_service_id =
      settings.value(QStringLiteral("serviceId"), serviceId()).toString().trimmed();
  if (!stored_service_id.isEmpty()) {
    service_id_ = stored_service_id.toStdString();
  }

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
  settings.setValue(QStringLiteral("bluetoothEnabled"), bluetooth_enabled_);
  settings.setValue(QStringLiteral("bleEnabled"), ble_enabled_);
  settings.setValue(QStringLiteral("wifiLanEnabled"), wifi_lan_enabled_);
  settings.setValue(QStringLiteral("wifiHotspotEnabled"), wifi_hotspot_enabled_);
  settings.setValue(QStringLiteral("webRtcEnabled"), web_rtc_enabled_);
  settings.setValue(QStringLiteral("connectionStrategy"), connection_strategy_);
  settings.setValue(QStringLiteral("serviceId"), serviceId());
  settings.setValue(QStringLiteral("logPath"), log_path_);
  settings.sync();
}

void FileShareTrayController::setDeviceName(const QString& device_name) {
  const QString trimmed = device_name.trimmed();
  if (trimmed.isEmpty() || trimmed == device_name_) {
    return;
  }

  device_name_ = trimmed;
  emit deviceNameChanged();
  SaveSettings();
  LogLine(QStringLiteral("Device name changed to %1").arg(device_name_));

  if (running_) {
    stop();
    start();
  }
}

void FileShareTrayController::setAutoAcceptIncoming(bool enabled) {
  if (auto_accept_incoming_ == enabled) return;
  auto_accept_incoming_ = enabled;
  emit autoAcceptIncomingChanged();
  SaveSettings();
}

void FileShareTrayController::setBluetoothEnabled(bool enabled) {
  if (bluetooth_enabled_ == enabled) return;
  bluetooth_enabled_ = enabled;
  emit bluetoothEnabledChanged();
  SaveSettings();
}

void FileShareTrayController::setBleEnabled(bool enabled) {
  if (ble_enabled_ == enabled) return;
  ble_enabled_ = enabled;
  emit bleEnabledChanged();
  SaveSettings();
}

void FileShareTrayController::setWifiLanEnabled(bool enabled) {
  if (wifi_lan_enabled_ == enabled) return;
  wifi_lan_enabled_ = enabled;
  emit wifiLanEnabledChanged();
  SaveSettings();
}

void FileShareTrayController::setWifiHotspotEnabled(bool enabled) {
  if (wifi_hotspot_enabled_ == enabled) return;
  wifi_hotspot_enabled_ = enabled;
  emit wifiHotspotEnabledChanged();
  SaveSettings();
}

void FileShareTrayController::setWebRtcEnabled(bool enabled) {
  if (web_rtc_enabled_ == enabled) return;
  web_rtc_enabled_ = enabled;
  emit webRtcEnabledChanged();
  SaveSettings();
}

void FileShareTrayController::setConnectionStrategy(const QString& strategy) {
  const QString normalized = NormalizeConnectionStrategy(strategy);
  if (connection_strategy_ == normalized) return;
  connection_strategy_ = normalized;
  emit connectionStrategyChanged();
  SaveSettings();
}

void FileShareTrayController::setServiceId(const QString& service_id) {
  const QString trimmed = service_id.trimmed();
  if (trimmed.isEmpty() || trimmed.toStdString() == service_id_) return;
  service_id_ = trimmed.toStdString();
  emit serviceIdChanged();
  SaveSettings();
}

void FileShareTrayController::setLogPath(const QString& path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty() || trimmed == log_path_) return;
  log_path_ = trimmed;
  emit logPathChanged();
  SaveSettings();
  ReopenLogFile();
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

  service_.StopDiscovery(
      service_id_, [this](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              LogLine(QStringLiteral("StopDiscovery: %1")
                          .arg(StatusToString(status)));
            },
            Qt::QueuedConnection);
      });

  service_.StopAdvertising(
      service_id_, [this](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              LogLine(QStringLiteral("StopAdvertising: %1")
                          .arg(StatusToString(status)));
            },
            Qt::QueuedConnection);
      });

  service_.StopAllEndpoints([this](NearbyConnectionsQtFacade::Status status) {
    QMetaObject::invokeMethod(
        this,
        [this, status]() {
          LogLine(QStringLiteral("StopAllEndpoints: %1")
                      .arg(StatusToString(status)));
        },
        Qt::QueuedConnection);
  });

  discovered_devices_.clear();
  connected_devices_.clear();
  endpoint_peer_names_.clear();
  endpoint_mediums_.clear();
  target_endpoint_for_send_.clear();

  emit discoveredDevicesChanged();
  emit connectedDevicesChanged();
  emit endpointMediumsChanged();

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
    LogLine(QStringLiteral("Mode changed to Receive"));
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
    LogLine(QStringLiteral("Mode changed to Send"));
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

void FileShareTrayController::sendPendingFileToEndpoint(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }

  if (pending_send_file_path_.isEmpty()) {
    SetStatus(QStringLiteral("No file selected"));
    emit requestTrayMessage(QStringLiteral("Send failed"),
                            QStringLiteral("Select a file first."));
    return;
  }

  target_endpoint_for_send_ = endpoint;

  if (connected_devices_.contains(endpoint)) {
    sendPendingFile(endpoint);
    return;
  }

  requestConnectionForSend(endpoint);
}

QString FileShareTrayController::mediumForEndpoint(const QString& endpoint_id) const {
  return endpoint_mediums_.value(endpoint_id).toString();
}

QString FileShareTrayController::peerNameForEndpoint(const QString& endpoint_id) const {
  return PeerLabelForEndpoint(endpoint_id);
}

void FileShareTrayController::clearTransfers() {
  transfers_.clear();
  transfer_row_for_payload_.clear();
  outgoing_file_payload_to_endpoint_.clear();
  outgoing_file_payload_to_name_.clear();
  send_terminal_notified_.clear();
  emit transfersChanged();
}

void FileShareTrayController::hideToTray() {
  emit requestTrayMessage(
      QStringLiteral("Nearby File Tray"),
      QStringLiteral("App is still running in the system tray."));
}

void FileShareTrayController::startSendMode() {
  discovered_devices_.clear();
  emit discoveredDevicesChanged();

  service_.StartDiscovery(
      service_id_, BuildDiscoveryOptions(), BuildDiscoveryListener(),
      [this](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              SetStatus(QStringLiteral("StartDiscovery: %1")
                            .arg(StatusToString(status)));
              LogLine(QStringLiteral("StartDiscovery: %1")
                          .arg(StatusToString(status)));
              if (status != NearbyConnectionsQtFacade::Status::kSuccess) {
                running_ = false;
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::startReceiveMode() {
  service_.StartAdvertising(
      service_id_, BuildEndpointInfo(), BuildAdvertisingOptions(),
      BuildConnectionListener(),
      [this](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, status]() {
              SetStatus(QStringLiteral("StartAdvertising: %1")
                            .arg(StatusToString(status)));
              LogLine(QStringLiteral("StartAdvertising: %1")
                          .arg(StatusToString(status)));
              if (status != NearbyConnectionsQtFacade::Status::kSuccess) {
                running_ = false;
                emit runningChanged();
              }
            },
            Qt::QueuedConnection);
      });
}

std::vector<uint8_t> FileShareTrayController::BuildEndpointInfo() const {
  QByteArray endpoint = device_name_.toUtf8();
  return std::vector<uint8_t>(endpoint.begin(), endpoint.end());
}

NearbyConnectionsQtFacade::ConnectionListener
FileShareTrayController::BuildConnectionListener() {
  NearbyConnectionsQtFacade::ConnectionListener listener;

  listener.initiated_cb = [this](const std::string& endpoint_id,
                                 const NearbyConnectionsQtFacade::ConnectionInfo& info) {
    QMetaObject::invokeMethod(
        this,
        [this, endpoint = QString::fromStdString(endpoint_id),
         peer_name = QString::fromStdString(info.peer_name),
         incoming = info.is_incoming_connection]() {
          SetPeerNameForEndpoint(endpoint, peer_name);
          const QString peer = PeerLabelForEndpoint(endpoint);

          if (incoming) {
            SetStatus(QStringLiteral("Incoming connection from %1").arg(peer));
            if (auto_accept_incoming_) {
              acceptIncomingInternal(endpoint);
            }
          } else {
            // Always accept outgoing connections (initiated by us for send).
            acceptIncomingInternal(endpoint);
          }
        },
        Qt::QueuedConnection);
  };

  listener.accepted_cb = [this](const std::string& endpoint_id) {
    QMetaObject::invokeMethod(
        this,
        [this, endpoint = QString::fromStdString(endpoint_id)]() {
          const QString peer = PeerLabelForEndpoint(endpoint);
          AddConnectedDevice(endpoint);
          SetStatus(QStringLiteral("Connected to %1").arg(peer));
          LogLine(QStringLiteral("Connection accepted endpoint=%1 peer=%2")
                      .arg(endpoint, peer));

          if (!target_endpoint_for_send_.isEmpty() &&
              target_endpoint_for_send_ == endpoint &&
              !pending_send_file_path_.isEmpty()) {
            sendPendingFile(endpoint);
          }
        },
        Qt::QueuedConnection);
  };

  listener.rejected_cb =
      [this](const std::string& endpoint_id, NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id), status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              SetStatus(QStringLiteral("Connection rejected by %1 (%2)")
                            .arg(peer, StatusToString(status)));
              LogLine(QStringLiteral("Connection rejected endpoint=%1 status=%2")
                          .arg(endpoint, StatusToString(status)));
            },
            Qt::QueuedConnection);
      };

  listener.disconnected_cb = [this](const std::string& endpoint_id) {
    QMetaObject::invokeMethod(
        this,
        [this, endpoint = QString::fromStdString(endpoint_id)]() {
          const QString peer = PeerLabelForEndpoint(endpoint);
          RemoveConnectedDevice(endpoint);
          endpoint_mediums_.remove(endpoint);
          emit endpointMediumsChanged();
          SetStatus(QStringLiteral("Disconnected from %1").arg(peer));
          LogLine(QStringLiteral("Disconnected endpoint=%1").arg(endpoint));
        },
        Qt::QueuedConnection);
  };

  listener.bandwidth_changed_cb =
      [this](const std::string& endpoint_id, NearbyConnectionsQtFacade::Medium medium) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id), medium]() {
              const QString medium_name = MediumToString(medium);
              endpoint_mediums_[endpoint] = medium_name;
              emit endpointMediumsChanged();
              UpdateTransferMediumForEndpoint(endpoint, medium_name);
              LogLine(QStringLiteral("Bandwidth changed endpoint=%1 medium=%2")
                          .arg(endpoint, medium_name));
            },
            Qt::QueuedConnection);
      };

  return listener;
}

NearbyConnectionsQtFacade::DiscoveryListener
FileShareTrayController::BuildDiscoveryListener() {
  NearbyConnectionsQtFacade::DiscoveryListener listener;

  listener.endpoint_found_cb =
      [this](const std::string& endpoint_id,
             const NearbyConnectionsQtFacade::DiscoveredEndpointInfo& info) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id),
             peer_name = QString::fromStdString(info.peer_name)]() {
              SetPeerNameForEndpoint(endpoint, peer_name);
              AddDiscoveredDevice(endpoint);
              LogLine(QStringLiteral("Discovered endpoint=%1 peer=%2")
                          .arg(endpoint, PeerLabelForEndpoint(endpoint)));
            },
            Qt::QueuedConnection);
      };

  listener.endpoint_lost_cb = [this](const std::string& endpoint_id) {
    QMetaObject::invokeMethod(
        this,
        [this, endpoint = QString::fromStdString(endpoint_id)]() {
          RemoveDiscoveredDevice(endpoint);
          LogLine(QStringLiteral("Lost endpoint=%1").arg(endpoint));
        },
        Qt::QueuedConnection);
  };

  listener.endpoint_distance_changed_cb =
      [this](const std::string& endpoint_id, NearbyConnectionsQtFacade::DistanceInfo info) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id), info]() {
              LogLine(QStringLiteral("Distance changed endpoint=%1 value=%2")
                          .arg(endpoint)
                          .arg(static_cast<int>(info)));
            },
            Qt::QueuedConnection);
      };

  return listener;
}

NearbyConnectionsQtFacade::PayloadListener
FileShareTrayController::BuildPayloadListener() {
  NearbyConnectionsQtFacade::PayloadListener listener;

  listener.payload_cb =
      [this](const std::string& endpoint_id, NearbyConnectionsQtFacade::Payload payload) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id),
             payload = std::move(payload)]() {
              if (payload.type == NearbyConnectionsQtFacade::Payload::Type::kBytes) {
                const QString text = QString::fromUtf8(
                    reinterpret_cast<const char*>(payload.bytes.data()),
                    static_cast<int>(payload.bytes.size()));
                if (text.startsWith(QStringLiteral("FILE:"))) {
                  const QString filename = text.mid(5).trimmed();
                  if (!filename.isEmpty()) {
                    pending_file_names_[endpoint] = filename;
                  }
                }
                return;
              }

              if (payload.type != NearbyConnectionsQtFacade::Payload::Type::kFile) {
                return;
              }

              QString file_name = QString::fromStdString(payload.file_name).trimmed();
              if (pending_file_names_.contains(endpoint)) {
                file_name = pending_file_names_.take(endpoint);
              }
              incoming_file_endpoints_[payload.id] = endpoint;
              incoming_file_names_[payload.id] = file_name;
              incoming_file_paths_[payload.id] =
                  QString::fromStdString(payload.file_path);

              LogLine(QStringLiteral("Incoming file payload announced endpoint=%1 id=%2 name=%3 path=%4")
                          .arg(endpoint)
                          .arg(payload.id)
                          .arg(file_name, QString::fromStdString(payload.file_path)));
            },
            Qt::QueuedConnection);
      };

  listener.payload_progress_cb =
      [this](const std::string& endpoint_id,
             const NearbyConnectionsQtFacade::PayloadTransferUpdate& update) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id), update]() {
              const bool is_outgoing_file =
                  outgoing_file_payload_to_endpoint_.contains(update.payload_id);
              const QString direction =
                  is_outgoing_file ? QStringLiteral("outgoing") : QStringLiteral("incoming");

              UpsertTransfer(endpoint, update.payload_id,
                             PayloadStatusToString(update.status),
                             update.bytes_transferred, update.total_bytes,
                             direction);

              if (!is_outgoing_file || !IsTerminalPayloadStatus(update.status) ||
                  send_terminal_notified_.contains(update.payload_id)) {
                if (!is_outgoing_file &&
                    update.status == NearbyConnectionsQtFacade::PayloadStatus::kSuccess &&
                    incoming_file_paths_.contains(update.payload_id)) {
                  const QString received_path = incoming_file_paths_.take(update.payload_id);
                  const QString received_name = incoming_file_names_.take(update.payload_id);
                  const QString incoming_endpoint =
                      incoming_file_endpoints_.take(update.payload_id);
                  const QString final_path = FinalizeReceivedFilePath(
                      received_path, received_name, update.payload_id);
                  const QString peer = PeerLabelForEndpoint(incoming_endpoint);
                  const QString final_name = QFileInfo(final_path).fileName();
                  emit requestTrayMessage(
                      QStringLiteral("File received"),
                      QStringLiteral("%1 from %2").arg(final_name, peer));
                  LogLine(QStringLiteral("Received file endpoint=%1 id=%2 saved=%3")
                              .arg(incoming_endpoint)
                              .arg(update.payload_id)
                              .arg(final_path));
                } else if (!is_outgoing_file &&
                           IsTerminalPayloadStatus(update.status)) {
                  incoming_file_paths_.remove(update.payload_id);
                  incoming_file_names_.remove(update.payload_id);
                  incoming_file_endpoints_.remove(update.payload_id);
                }
                return;
              }

              send_terminal_notified_.insert(update.payload_id);

              const QString peer = PeerLabelForEndpoint(endpoint);
              const QString file_name =
                  outgoing_file_payload_to_name_.value(update.payload_id,
                                                       QStringLiteral("file"));

              if (update.status == NearbyConnectionsQtFacade::PayloadStatus::kSuccess) {
                emit requestTrayMessage(
                    QStringLiteral("Send complete"),
                    QStringLiteral("%1 sent to %2").arg(file_name, peer));
              } else {
                emit requestTrayMessage(
                    QStringLiteral("Send failed"),
                    QStringLiteral("%1 failed to send to %2")
                        .arg(file_name, peer));
              }

              outgoing_file_payload_to_endpoint_.remove(update.payload_id);
              outgoing_file_payload_to_name_.remove(update.payload_id);
              send_terminal_notified_.remove(update.payload_id);

              if (!pending_send_file_path_.isEmpty()) {
                pending_send_file_path_.clear();
                emit pendingSendFilePathChanged();
              }
              if (!pending_send_file_name_.isEmpty()) {
                pending_send_file_name_.clear();
                emit pendingSendFileNameChanged();
              }
              target_endpoint_for_send_.clear();

              disconnectDevice(endpoint);
            },
            Qt::QueuedConnection);
      };

  return listener;
}

NearbyConnectionsQtFacade::MediumSelection
FileShareTrayController::BuildMediumSelection() const {
  NearbyConnectionsQtFacade::MediumSelection selection;
  selection.bluetooth = bluetooth_enabled_;
  selection.ble = ble_enabled_;
  selection.wifi_lan = wifi_lan_enabled_;
  selection.wifi_hotspot = wifi_hotspot_enabled_;
  selection.web_rtc = web_rtc_enabled_;
  return selection;
}

NearbyConnectionsQtFacade::AdvertisingOptions
FileShareTrayController::BuildAdvertisingOptions() const {
  NearbyConnectionsQtFacade::AdvertisingOptions options;
  options.strategy = StrategyFromName(connection_strategy_);
  options.allowed_mediums = BuildMediumSelection();
  options.auto_upgrade_bandwidth = true;
  options.enable_bluetooth_listening = true;
  options.enforce_topology_constraints = true;
  return options;
}

NearbyConnectionsQtFacade::DiscoveryOptions
FileShareTrayController::BuildDiscoveryOptions() const {
  NearbyConnectionsQtFacade::DiscoveryOptions options;
  options.strategy = StrategyFromName(connection_strategy_);
  options.allowed_mediums = BuildMediumSelection();
  return options;
}

NearbyConnectionsQtFacade::ConnectionOptions
FileShareTrayController::BuildConnectionOptions() const {
  NearbyConnectionsQtFacade::ConnectionOptions options;
  options.allowed_mediums = BuildMediumSelection();
  options.non_disruptive_hotspot_mode = true;
  return options;
}

void FileShareTrayController::acceptIncomingInternal(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }

  service_.AcceptConnection(
      service_id_, endpoint.toStdString(), BuildPayloadListener(),
      [this, endpoint](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              SetStatus(QStringLiteral("AcceptConnection(%1): %2")
                            .arg(peer, StatusToString(status)));
              LogLine(QStringLiteral("AcceptConnection(%1): %2")
                          .arg(endpoint, StatusToString(status)));
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::requestConnectionForSend(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }

  const QString peer = PeerLabelForEndpoint(endpoint);
  SetStatus(QStringLiteral("Requesting connection to %1").arg(peer));
  LogLine(QStringLiteral("RequestConnection %1").arg(endpoint));

  service_.RequestConnection(
      service_id_, BuildEndpointInfo(), endpoint.toStdString(),
      BuildConnectionOptions(), BuildConnectionListener(),
      [this, endpoint](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              SetStatus(QStringLiteral("RequestConnection(%1): %2")
                            .arg(peer, StatusToString(status)));
              LogLine(QStringLiteral("RequestConnection(%1): %2")
                          .arg(endpoint, StatusToString(status)));
              if (status != NearbyConnectionsQtFacade::Status::kSuccess) {
                emit requestTrayMessage(
                    QStringLiteral("Send failed"),
                    QStringLiteral("Could not connect to %1").arg(peer));
              }
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::sendPendingFile(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  QFileInfo file_info(pending_send_file_path_);
  if (endpoint.isEmpty() || pending_send_file_path_.isEmpty() ||
      !file_info.exists() || !file_info.isFile()) {
    SetStatus(QStringLiteral("Selected file is not available"));
    emit requestTrayMessage(QStringLiteral("Send failed"),
                            QStringLiteral("Selected file is not available."));
    return;
  }

  const QString peer = PeerLabelForEndpoint(endpoint);
  const QString file_name =
      pending_send_file_name_.isEmpty() ? file_info.fileName()
                                        : pending_send_file_name_;

  // Send metadata message first to preserve file names on receiver side.
  const QString metadata = QStringLiteral("FILE:%1").arg(file_name);
  QByteArray metadata_bytes = metadata.toUtf8();
  std::vector<uint8_t> metadata_vec(metadata_bytes.begin(), metadata_bytes.end());
  auto metadata_payload = service_.CreateBytesPayload(std::move(metadata_vec));

  service_.SendPayload(
      service_id_, {endpoint.toStdString()}, std::move(metadata_payload),
      [this, endpoint](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, status]() {
              LogLine(QStringLiteral("Send metadata payload (%1): %2")
                          .arg(endpoint, StatusToString(status)));
            },
            Qt::QueuedConnection);
      });

  NearbyConnectionsQtFacade::Payload file_payload;
  file_payload.id = g_local_payload_id.fetch_add(1);
  file_payload.type = NearbyConnectionsQtFacade::Payload::Type::kFile;
  file_payload.file_path = file_info.absoluteFilePath().toStdString();
  file_payload.file_name = file_name.toStdString();
  file_payload.parent_folder = "";

  const qlonglong payload_id = file_payload.id;
  const qulonglong total_bytes =
      static_cast<qulonglong>(qMax<qint64>(0, file_info.size()));

  UpsertTransfer(endpoint, payload_id, QStringLiteral("Queued"), 0,
                 total_bytes, QStringLiteral("outgoing"));

  outgoing_file_payload_to_endpoint_.insert(payload_id, endpoint);
  outgoing_file_payload_to_name_.insert(payload_id, file_name);
  send_terminal_notified_.remove(payload_id);

  emit requestTrayMessage(
      QStringLiteral("Sending file"),
      QStringLiteral("Sending %1 to %2").arg(file_name, peer));

  service_.SendPayload(
      service_id_, {endpoint.toStdString()}, std::move(file_payload),
      [this, endpoint, payload_id, file_name,
       total_bytes](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, payload_id, file_name, status, total_bytes]() {
              LogLine(QStringLiteral("Send file payload (%1, %2): %3")
                          .arg(endpoint)
                          .arg(payload_id)
                          .arg(StatusToString(status)));

              if (status == NearbyConnectionsQtFacade::Status::kSuccess) {
                SetStatus(QStringLiteral("Sending %1...").arg(file_name));
                return;
              }

              UpsertTransfer(endpoint, payload_id, QStringLiteral("SendFailed"),
                             0, total_bytes, QStringLiteral("outgoing"));

              const QString peer = PeerLabelForEndpoint(endpoint);
              emit requestTrayMessage(
                  QStringLiteral("Send failed"),
                  QStringLiteral("%1 failed to send to %2")
                      .arg(file_name, peer));

              outgoing_file_payload_to_endpoint_.remove(payload_id);
              outgoing_file_payload_to_name_.remove(payload_id);
              send_terminal_notified_.remove(payload_id);

              if (!pending_send_file_path_.isEmpty()) {
                pending_send_file_path_.clear();
                emit pendingSendFilePathChanged();
              }
              if (!pending_send_file_name_.isEmpty()) {
                pending_send_file_name_.clear();
                emit pendingSendFileNameChanged();
              }

              target_endpoint_for_send_.clear();
              disconnectDevice(endpoint);
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::disconnectDevice(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }

  service_.DisconnectFromEndpoint(
      service_id_, endpoint.toStdString(),
      [this, endpoint](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              LogLine(QStringLiteral("Disconnect(%1): %2")
                          .arg(endpoint, StatusToString(status)));
              if (status == NearbyConnectionsQtFacade::Status::kSuccess) {
                RemoveConnectedDevice(endpoint);
                endpoint_mediums_.remove(endpoint);
                emit endpointMediumsChanged();
                SetStatus(QStringLiteral("Disconnected from %1").arg(peer));
              }
            },
            Qt::QueuedConnection);
      });
}

void FileShareTrayController::AddDiscoveredDevice(const QString& endpoint_id) {
  if (discovered_devices_.contains(endpoint_id)) {
    return;
  }
  discovered_devices_.append(endpoint_id);
  emit discoveredDevicesChanged();
}

void FileShareTrayController::RemoveDiscoveredDevice(const QString& endpoint_id) {
  if (!discovered_devices_.removeOne(endpoint_id)) {
    return;
  }
  emit discoveredDevicesChanged();
}

void FileShareTrayController::AddConnectedDevice(const QString& endpoint_id) {
  if (connected_devices_.contains(endpoint_id)) {
    return;
  }
  connected_devices_.append(endpoint_id);
  emit connectedDevicesChanged();
}

void FileShareTrayController::RemoveConnectedDevice(const QString& endpoint_id) {
  if (!connected_devices_.removeOne(endpoint_id)) {
    return;
  }
  emit connectedDevicesChanged();
}

void FileShareTrayController::SetPeerNameForEndpoint(const QString& endpoint_id,
                                                     const QString& peer_name) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }

  const QString trimmed_name = peer_name.trimmed();
  const QString previous = endpoint_peer_names_.value(endpoint).trimmed();
  if (previous == trimmed_name) {
    return;
  }

  if (trimmed_name.isEmpty()) {
    endpoint_peer_names_.remove(endpoint);
  } else {
    endpoint_peer_names_[endpoint] = trimmed_name;
  }

  emit discoveredDevicesChanged();
  emit connectedDevicesChanged();
}

QString FileShareTrayController::PeerLabelForEndpoint(const QString& endpoint_id) const {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return QStringLiteral("Unknown device");
  }

  const QString peer_name = endpoint_peer_names_.value(endpoint).trimmed();
  return peer_name.isEmpty() ? QStringLiteral("Unknown device") : peer_name;
}

QString FileShareTrayController::FinalizeReceivedFilePath(
    const QString& received_path, const QString& received_file_name,
    qlonglong payload_id) const {
  const QString source = received_path.trimmed();
  if (source.isEmpty()) {
    return source;
  }

  QFileInfo source_info(source);
  const QString source_abs = source_info.absoluteFilePath();
  const QString source_dir = source_info.absolutePath();

  QString target_name = QFileInfo(received_file_name.trimmed()).fileName();
  if (target_name.isEmpty()) {
    target_name = source_info.fileName();
  }
  if (target_name.isEmpty()) {
    target_name = QStringLiteral("payload_%1.bin").arg(payload_id);
  }

  const QFileInfo target_name_info(target_name);
  const QString stem =
      target_name_info.completeBaseName().isEmpty()
          ? target_name_info.fileName()
          : target_name_info.completeBaseName();
  const QString suffix = target_name_info.completeSuffix();
  QString target_path = QDir(source_dir).filePath(target_name);

  int suffix_index = 1;
  while (target_path != source_abs && QFileInfo::exists(target_path)) {
    const QString next_name =
        suffix.isEmpty()
            ? QStringLiteral("%1_%2").arg(stem).arg(suffix_index)
            : QStringLiteral("%1_%2.%3")
                  .arg(stem)
                  .arg(suffix_index)
                  .arg(suffix);
    target_path = QDir(source_dir).filePath(next_name);
    ++suffix_index;
  }

  if (target_path == source_abs) {
    return source_abs;
  }

  if (QFile::rename(source_abs, target_path)) {
    return target_path;
  }

  if (QFile::copy(source_abs, target_path)) {
    QFile::remove(source_abs);
    return target_path;
  }

  return source_abs;
}

void FileShareTrayController::UpsertTransfer(const QString& endpoint_id,
                                             qlonglong payload_id,
                                             const QString& status,
                                             qulonglong bytes_transferred,
                                             qulonglong total_bytes,
                                             const QString& direction) {
  const QString medium = mediumForEndpoint(endpoint_id);
  const double progress =
      total_bytes > 0
          ? static_cast<double>(bytes_transferred) /
                static_cast<double>(total_bytes)
          : 0.0;

  QVariantMap transfer{{QStringLiteral("payloadId"), payload_id},
                       {QStringLiteral("endpointId"), endpoint_id},
                       {QStringLiteral("status"), status},
                       {QStringLiteral("bytesTransferred"), bytes_transferred},
                       {QStringLiteral("totalBytes"), total_bytes},
                       {QStringLiteral("progress"), progress},
                       {QStringLiteral("medium"), medium},
                       {QStringLiteral("direction"), direction}};

  if (transfer_row_for_payload_.contains(payload_id)) {
    const int row = transfer_row_for_payload_.value(payload_id);
    if (row >= 0 && row < transfers_.size()) {
      transfers_[row] = transfer;
      emit transfersChanged();
      return;
    }
  }

  transfer_row_for_payload_.insert(payload_id, transfers_.size());
  transfers_.append(transfer);
  emit transfersChanged();
}

void FileShareTrayController::UpdateTransferMediumForEndpoint(
    const QString& endpoint_id, const QString& medium) {
  bool changed = false;
  for (int i = 0; i < transfers_.size(); ++i) {
    QVariantMap row = transfers_[i].toMap();
    if (row.value(QStringLiteral("endpointId")).toString() != endpoint_id) {
      continue;
    }
    row[QStringLiteral("medium")] = medium;
    transfers_[i] = row;
    changed = true;
  }
  if (changed) {
    emit transfersChanged();
  }
}

void FileShareTrayController::SetStatus(const QString& status) {
  if (status == status_message_) {
    return;
  }
  status_message_ = status;
  emit statusMessageChanged();
  LogLine(QStringLiteral("Status: %1").arg(status_message_));
}

bool FileShareTrayController::HasActiveTransfers() const {
  for (const QVariant& row_value : transfers_) {
    const QVariantMap row = row_value.toMap();
    const QString status = row.value(QStringLiteral("status")).toString();
    if (status == QStringLiteral("InProgress") ||
        status == QStringLiteral("Queued")) {
      return true;
    }
  }
  return false;
}

void FileShareTrayController::LogLine(const QString& line) {
  if (!log_file_.isOpen()) {
    ReopenLogFile();
  }
  if (!log_file_.isOpen()) {
    return;
  }

  QTextStream stream(&log_file_);
  stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << " " << line
         << "\n";
  stream.flush();
}

void FileShareTrayController::ReopenLogFile() {
  if (log_file_.isOpen()) {
    log_file_.close();
  }
  log_file_.setFileName(log_path_);
  log_file_.open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly);
}

QString FileShareTrayController::StatusToString(
    NearbyConnectionsQtFacade::Status status) {
  switch (status) {
    case NearbyConnectionsQtFacade::Status::kSuccess:
      return QStringLiteral("Success");
    case NearbyConnectionsQtFacade::Status::kError:
      return QStringLiteral("Error");
    case NearbyConnectionsQtFacade::Status::kOutOfOrderApiCall:
      return QStringLiteral("OutOfOrderApiCall");
    case NearbyConnectionsQtFacade::Status::kAlreadyHaveActiveStrategy:
      return QStringLiteral("AlreadyHaveActiveStrategy");
    case NearbyConnectionsQtFacade::Status::kAlreadyAdvertising:
      return QStringLiteral("AlreadyAdvertising");
    case NearbyConnectionsQtFacade::Status::kAlreadyDiscovering:
      return QStringLiteral("AlreadyDiscovering");
    case NearbyConnectionsQtFacade::Status::kAlreadyListening:
      return QStringLiteral("AlreadyListening");
    case NearbyConnectionsQtFacade::Status::kEndpointIOError:
      return QStringLiteral("EndpointIOError");
    case NearbyConnectionsQtFacade::Status::kEndpointUnknown:
      return QStringLiteral("EndpointUnknown");
    case NearbyConnectionsQtFacade::Status::kConnectionRejected:
      return QStringLiteral("ConnectionRejected");
    case NearbyConnectionsQtFacade::Status::kAlreadyConnectedToEndpoint:
      return QStringLiteral("AlreadyConnectedToEndpoint");
    case NearbyConnectionsQtFacade::Status::kNotConnectedToEndpoint:
      return QStringLiteral("NotConnectedToEndpoint");
    case NearbyConnectionsQtFacade::Status::kBluetoothError:
      return QStringLiteral("BluetoothError");
    case NearbyConnectionsQtFacade::Status::kBleError:
      return QStringLiteral("BleError");
    case NearbyConnectionsQtFacade::Status::kWifiLanError:
      return QStringLiteral("WifiLanError");
    case NearbyConnectionsQtFacade::Status::kPayloadUnknown:
      return QStringLiteral("PayloadUnknown");
    case NearbyConnectionsQtFacade::Status::kReset:
      return QStringLiteral("Reset");
    case NearbyConnectionsQtFacade::Status::kTimeout:
      return QStringLiteral("Timeout");
    case NearbyConnectionsQtFacade::Status::kUnknown:
      return QStringLiteral("Unknown");
    case NearbyConnectionsQtFacade::Status::kNextValue:
      return QStringLiteral("NextValue");
  }
  return QStringLiteral("Unknown");
}

QString FileShareTrayController::PayloadStatusToString(
    NearbyConnectionsQtFacade::PayloadStatus status) {
  switch (status) {
    case NearbyConnectionsQtFacade::PayloadStatus::kSuccess:
      return QStringLiteral("Success");
    case NearbyConnectionsQtFacade::PayloadStatus::kFailure:
      return QStringLiteral("Failure");
    case NearbyConnectionsQtFacade::PayloadStatus::kInProgress:
      return QStringLiteral("InProgress");
    case NearbyConnectionsQtFacade::PayloadStatus::kCanceled:
      return QStringLiteral("Canceled");
  }
  return QStringLiteral("Unknown");
}

QString FileShareTrayController::MediumToString(NearbyConnectionsQtFacade::Medium medium) {
  switch (medium) {
    case NearbyConnectionsQtFacade::Medium::kUnknown:
      return QStringLiteral("Unknown");
    case NearbyConnectionsQtFacade::Medium::kMdns:
      return QStringLiteral("mDNS");
    case NearbyConnectionsQtFacade::Medium::kBluetooth:
      return QStringLiteral("Bluetooth");
    case NearbyConnectionsQtFacade::Medium::kWifiHotspot:
      return QStringLiteral("WiFiHotspot");
    case NearbyConnectionsQtFacade::Medium::kBle:
      return QStringLiteral("BLE");
    case NearbyConnectionsQtFacade::Medium::kWifiLan:
      return QStringLiteral("WiFiLAN");
    case NearbyConnectionsQtFacade::Medium::kWifiAware:
      return QStringLiteral("WiFiAware");
    case NearbyConnectionsQtFacade::Medium::kNfc:
      return QStringLiteral("NFC");
    case NearbyConnectionsQtFacade::Medium::kWifiDirect:
      return QStringLiteral("WiFiDirect");
    case NearbyConnectionsQtFacade::Medium::kWebRtc:
      return QStringLiteral("WebRTC");
    case NearbyConnectionsQtFacade::Medium::kBleL2Cap:
      return QStringLiteral("BLEL2CAP");
  }
  return QStringLiteral("Unknown");
}
