#include "sharing/linux/qml_tray_app/nearby_tray_controller.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QSettings>
#include <QSysInfo>
#include <QTextStream>

#include <memory>

namespace {

using NearbyConnectionsQtFacade =
    nearby::sharing::linux::NearbyConnectionsQtFacade;

QString NormalizeMediumsMode(QString mode) {
  mode = mode.trimmed().toLower();
  if (mode == QStringLiteral("all") || mode == QStringLiteral("bluetooth") ||
      mode == QStringLiteral("ble") || mode == QStringLiteral("wifi_lan") ||
      mode == QStringLiteral("wifi_hotspot") ||
      mode == QStringLiteral("web_rtc") ||
      mode == QStringLiteral("balanced")) {
    return mode;
  }
  return QStringLiteral("balanced");
}

NearbyConnectionsQtFacade::MediumSelection BuildMediumSelectionForMode(
    const QString& normalized_mode) {
  NearbyConnectionsQtFacade::MediumSelection selection;
  selection.bluetooth = false;
  selection.ble = false;
  selection.web_rtc = false;
  selection.wifi_lan = false;
  selection.wifi_hotspot = false;

  if (normalized_mode == QStringLiteral("all")) {
    selection.bluetooth = true;
    selection.ble = true;
    selection.web_rtc = true;
    selection.wifi_lan = true;
    selection.wifi_hotspot = true;
  } else if (normalized_mode == QStringLiteral("bluetooth")) {
    selection.bluetooth = true;
  } else if (normalized_mode == QStringLiteral("ble")) {
    selection.ble = true;
  } else if (normalized_mode == QStringLiteral("wifi_lan")) {
    selection.wifi_lan = true;
  } else if (normalized_mode == QStringLiteral("wifi_hotspot")) {
    selection.wifi_hotspot = true;
  } else if (normalized_mode == QStringLiteral("web_rtc")) {
    selection.web_rtc = true;
  } else {
    // Balanced default mirrors previous behavior.
    selection.bluetooth = true;
    selection.ble = true;
    selection.wifi_lan = true;
  }
  return selection;
}

QString NormalizeConnectionStrategy(QString strategy) {
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

NearbyTrayController::NearbyTrayController(QObject* parent) : QObject(parent) {
  const QString host = QSysInfo::machineHostName();
  if (!host.isEmpty()) {
    device_name_ = host;
  }
  LoadSettings();
  ReopenLogFile();
  LogLine(QStringLiteral("Started Nearby tray controller"));
}

NearbyTrayController::~NearbyTrayController() {
  stop();
  if (log_file_.isOpen()) {
    log_file_.close();
  }
}

void NearbyTrayController::LoadSettings() {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlTrayApp"));

  const QString stored_mode =
      settings.value(QStringLiteral("mode"), mode_).toString();
  mode_ = stored_mode.trimmed().toLower() == QStringLiteral("send")
              ? QStringLiteral("Send")
              : QStringLiteral("Receive");

  const QString stored_device_name =
      settings.value(QStringLiteral("deviceName"), device_name_)
          .toString()
          .trimmed();
  if (!stored_device_name.isEmpty()) {
    device_name_ = stored_device_name;
  }

  const QString stored_service_id =
      settings
          .value(QStringLiteral("serviceId"),
                 QString::fromStdString(service_id_))
          .toString()
          .trimmed();
  if (!stored_service_id.isEmpty()) {
    service_id_ = stored_service_id.toStdString();
  }

  mediums_mode_ = NormalizeMediumsMode(
      settings.value(QStringLiteral("mediumsMode"), mediums_mode_).toString());
  bluetooth_enabled_ = settings
                           .value(QStringLiteral("bluetoothEnabled"),
                                  bluetooth_enabled_)
                           .toBool();
  ble_enabled_ = settings.value(QStringLiteral("bleEnabled"), ble_enabled_).toBool();
  wifi_lan_enabled_ = settings
                          .value(QStringLiteral("wifiLanEnabled"),
                                 wifi_lan_enabled_)
                          .toBool();
  wifi_hotspot_enabled_ = settings
                              .value(QStringLiteral("wifiHotspotEnabled"),
                                     wifi_hotspot_enabled_)
                              .toBool();
  web_rtc_enabled_ =
      settings.value(QStringLiteral("webRtcEnabled"), web_rtc_enabled_).toBool();
  auto_accept_incoming_ = settings
                              .value(QStringLiteral("autoAcceptIncoming"),
                                     auto_accept_incoming_)
                              .toBool();
  connection_strategy_ = NormalizeConnectionStrategy(
      settings
          .value(QStringLiteral("connectionStrategy"), connection_strategy_)
          .toString());

  const QString stored_log_path =
      settings.value(QStringLiteral("logPath"), log_path_).toString().trimmed();
  if (!stored_log_path.isEmpty()) {
    log_path_ = stored_log_path;
  }
}

void NearbyTrayController::SaveSettings() const {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlTrayApp"));
  settings.setValue(QStringLiteral("mode"), mode_);
  settings.setValue(QStringLiteral("deviceName"), device_name_);
  settings.setValue(QStringLiteral("serviceId"),
                    QString::fromStdString(service_id_));
  settings.setValue(QStringLiteral("mediumsMode"), mediums_mode_);
  settings.setValue(QStringLiteral("bluetoothEnabled"), bluetooth_enabled_);
  settings.setValue(QStringLiteral("bleEnabled"), ble_enabled_);
  settings.setValue(QStringLiteral("wifiLanEnabled"), wifi_lan_enabled_);
  settings.setValue(QStringLiteral("wifiHotspotEnabled"), wifi_hotspot_enabled_);
  settings.setValue(QStringLiteral("webRtcEnabled"), web_rtc_enabled_);
  settings.setValue(QStringLiteral("autoAcceptIncoming"), auto_accept_incoming_);
  settings.setValue(QStringLiteral("connectionStrategy"), connection_strategy_);
  settings.setValue(QStringLiteral("logPath"), log_path_);
  settings.sync();
}

void NearbyTrayController::setMode(const QString& mode) {
  const QString normalized =
      mode.trimmed().toLower() == QStringLiteral("send")
          ? QStringLiteral("Send")
          : QStringLiteral("Receive");
  if (mode_ == normalized) {
    return;
  }
  mode_ = normalized;
  emit modeChanged();
  SaveSettings();
  LogLine(QStringLiteral("Mode changed to %1").arg(mode_));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setDeviceName(const QString& device_name) {
  const QString trimmed = device_name.trimmed();
  if (trimmed.isEmpty() || trimmed == device_name_) {
    return;
  }
  device_name_ = trimmed;
  emit deviceNameChanged();
  SaveSettings();
  LogLine(QStringLiteral("Device name changed to %1").arg(device_name_));
}

void NearbyTrayController::setServiceId(const QString& service_id) {
  const std::string value = service_id.trimmed().toStdString();
  if (value.empty() || value == service_id_) {
    return;
  }
  service_id_ = value;
  emit serviceIdChanged();
  SaveSettings();
  LogLine(QStringLiteral("Service ID changed to %1")
              .arg(QString::fromStdString(service_id_)));
}

void NearbyTrayController::setMediumsMode(const QString& mode) {
  const QString normalized = NormalizeMediumsMode(mode);
  if (mediums_mode_ == normalized) {
    return;
  }
  mediums_mode_ = normalized;
  emit mediumsModeChanged();
  SaveSettings();
  LogLine(QStringLiteral("Mediums mode changed to %1").arg(mediums_mode_));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setBluetoothEnabled(bool enabled) {
  if (bluetooth_enabled_ == enabled) {
    return;
  }
  bluetooth_enabled_ = enabled;
  emit bluetoothEnabledChanged();
  SaveSettings();
  LogLine(QStringLiteral("Bluetooth %1").arg(enabled ? "enabled" : "disabled"));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setBleEnabled(bool enabled) {
  if (ble_enabled_ == enabled) {
    return;
  }
  ble_enabled_ = enabled;
  emit bleEnabledChanged();
  SaveSettings();
  LogLine(QStringLiteral("BLE %1").arg(enabled ? "enabled" : "disabled"));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setWifiLanEnabled(bool enabled) {
  if (wifi_lan_enabled_ == enabled) {
    return;
  }
  wifi_lan_enabled_ = enabled;
  emit wifiLanEnabledChanged();
  SaveSettings();
  LogLine(QStringLiteral("WiFi LAN %1").arg(enabled ? "enabled" : "disabled"));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setWifiHotspotEnabled(bool enabled) {
  if (wifi_hotspot_enabled_ == enabled) {
    return;
  }
  wifi_hotspot_enabled_ = enabled;
  emit wifiHotspotEnabledChanged();
  SaveSettings();
  LogLine(QStringLiteral("WiFi Hotspot %1").arg(enabled ? "enabled" : "disabled"));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setWebRtcEnabled(bool enabled) {
  if (web_rtc_enabled_ == enabled) {
    return;
  }
  web_rtc_enabled_ = enabled;
  emit webRtcEnabledChanged();
  SaveSettings();
  LogLine(QStringLiteral("WebRTC %1").arg(enabled ? "enabled" : "disabled"));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setAutoAcceptIncoming(bool enabled) {
  if (auto_accept_incoming_ == enabled) {
    return;
  }
  auto_accept_incoming_ = enabled;
  emit autoAcceptIncomingChanged();
  SaveSettings();
  LogLine(QStringLiteral("Auto-accept incoming connections %1")
              .arg(enabled ? "enabled" : "disabled"));
}

void NearbyTrayController::setConnectionStrategy(const QString& strategy) {
  const QString normalized = NormalizeConnectionStrategy(strategy);
  if (connection_strategy_ == normalized) {
    return;
  }
  connection_strategy_ = normalized;
  emit connectionStrategyChanged();
  SaveSettings();
  LogLine(QStringLiteral("Connection strategy changed to %1")
              .arg(connection_strategy_));
  if (running_) {
    stop();
    start();
  }
}

void NearbyTrayController::setLogPath(const QString& path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty() || trimmed == log_path_) {
    return;
  }
  log_path_ = trimmed;
  emit logPathChanged();
  SaveSettings();
  ReopenLogFile();
  LogLine(QStringLiteral("Log path changed to %1").arg(log_path_));
}

void NearbyTrayController::start() {
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

void NearbyTrayController::stop() {
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
  service_.StopAllEndpoints(
      [this](NearbyConnectionsQtFacade::Status status) {
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
  pending_connections_.clear();
  endpoint_peer_names_.clear();
  endpoint_mediums_.clear();
  emit discoveredDevicesChanged();
  emit connectedDevicesChanged();
  emit pendingConnectionsChanged();
  emit endpointMediumsChanged();

  SetStatus(QStringLiteral("Stopped"));
}

void NearbyTrayController::connectToDevice(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }
  const QString peer = PeerLabelForEndpoint(endpoint);
  if (!running_) {
    start();
  }

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
            },
            Qt::QueuedConnection);
      });
}

void NearbyTrayController::disconnectDevice(const QString& endpoint_id) {
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
              SetStatus(QStringLiteral("Disconnect(%1): %2")
                            .arg(peer, StatusToString(status)));
              LogLine(QStringLiteral("Disconnect(%1): %2")
                          .arg(endpoint, StatusToString(status)));
              if (status == NearbyConnectionsQtFacade::Status::kSuccess) {
                RemoveConnectedDevice(endpoint);
                RemovePendingConnection(endpoint);
                endpoint_peer_names_.remove(endpoint);
                endpoint_mediums_.remove(endpoint);
                emit endpointMediumsChanged();
              }
            },
            Qt::QueuedConnection);
      });
}

void NearbyTrayController::acceptIncoming(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }
  RemovePendingConnection(endpoint);

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

void NearbyTrayController::rejectIncoming(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }
  RemovePendingConnection(endpoint);
  disconnectDevice(endpoint);
}

void NearbyTrayController::initiateBandwidthUpgrade(const QString& endpoint_id) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }
  service_.InitiateBandwidthUpgrade(
      service_id_, endpoint.toStdString(),
      [this, endpoint](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              SetStatus(QStringLiteral("InitiateBandwidthUpgrade(%1): %2")
                            .arg(peer, StatusToString(status)));
              LogLine(QStringLiteral("InitiateBandwidthUpgrade(%1): %2")
                          .arg(endpoint, StatusToString(status)));
            },
            Qt::QueuedConnection);
      });
}

void NearbyTrayController::sendText(const QString& endpoint_id,
                                    const QString& text) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty() || text.isEmpty()) {
    return;
  }

  QByteArray utf8 = text.toUtf8();
  std::vector<uint8_t> bytes(utf8.begin(), utf8.end());
  auto payload = service_.CreateBytesPayload(std::move(bytes));
  const qlonglong payload_id = payload.id;

  UpsertTransfer(endpoint, payload_id, QStringLiteral("Queued"), 0,
                 static_cast<qulonglong>(bytes.size()),
                 QStringLiteral("outgoing"));

  const std::vector<std::string> endpoint_ids{endpoint.toStdString()};
  service_.SendPayload(
      service_id_, endpoint_ids, std::move(payload),
      [this, endpoint, payload_id](NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint, payload_id, status]() {
              LogLine(QStringLiteral("SendPayload(%1, %2): %3")
                          .arg(endpoint)
                          .arg(payload_id)
                          .arg(StatusToString(status)));
              if (status != NearbyConnectionsQtFacade::Status::kSuccess) {
                UpsertTransfer(endpoint, payload_id, QStringLiteral("SendFailed"),
                               0, 0, QStringLiteral("outgoing"));
              }
            },
            Qt::QueuedConnection);
      });
}

QString NearbyTrayController::mediumForEndpoint(const QString& endpoint_id) const {
  return endpoint_mediums_.value(endpoint_id).toString();
}

QString NearbyTrayController::peerNameForEndpoint(const QString& endpoint_id) const {
  return PeerLabelForEndpoint(endpoint_id);
}

void NearbyTrayController::clearTransfers() {
  transfers_.clear();
  transfer_row_for_payload_.clear();
  emit transfersChanged();
}

void NearbyTrayController::hideToTray() {
  emit requestTrayMessage(
      QStringLiteral("Nearby Tray"),
      QStringLiteral("App is still running in the system tray."));
}

void NearbyTrayController::startSendMode() {
  discovered_devices_.clear();
  emit discoveredDevicesChanged();
  auto discovery_options = BuildDiscoveryOptions();
  LogLine(QStringLiteral("ble: %1, bluetooth: %2, wifi_lan: %3, wifi_hotspot: $4").arg(
    discovery_options.allowed_mediums.ble
  ).arg(
    discovery_options.allowed_mediums.bluetooth
  ).arg(
    discovery_options.allowed_mediums.wifi_lan).arg(discovery_options.allowed_mediums.wifi_hotspot)
  );
  auto discovery_listener = BuildDiscoveryListener();
  service_.StartDiscovery(
      service_id_,discovery_options, discovery_listener,
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

void NearbyTrayController::startReceiveMode() {
  auto advert_options = BuildAdvertisingOptions();
  LogLine(QStringLiteral("ble: %1, bluetooth: %2, wifi_lan: %3, wifi_hotspot: %4").arg(
    advert_options.allowed_mediums.ble
  ).arg(
    advert_options.allowed_mediums.bluetooth
  ).arg(
    advert_options.allowed_mediums.wifi_lan
  ).arg(advert_options.allowed_mediums.wifi_hotspot));
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

std::vector<uint8_t> NearbyTrayController::BuildEndpointInfo() const {
  QByteArray endpoint = device_name_.toUtf8();
  return std::vector<uint8_t>(endpoint.begin(), endpoint.end());
}

NearbyConnectionsQtFacade::ConnectionListener
NearbyTrayController::BuildConnectionListener() {
  NearbyConnectionsQtFacade::ConnectionListener listener;
  listener.initiated_cb =
      [this](const std::string& endpoint_id,
             const NearbyConnectionsQtFacade::ConnectionInfo& info) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id),
             incoming = info.is_incoming_connection,
             peer_name = QString::fromStdString(info.peer_name)]() {
              SetPeerNameForEndpoint(endpoint, peer_name);
              const QString peer = PeerLabelForEndpoint(endpoint);
              LogLine(QStringLiteral("Connection initiated endpoint=%1 incoming=%2")
                          .arg(endpoint)
                          .arg(incoming));
              if (incoming) {
                AddPendingConnection(endpoint);
                SetStatus(
                    QStringLiteral("Incoming connection from %1").arg(peer));
                if (auto_accept_incoming_) {
                  acceptIncoming(endpoint);
                }
              } else {
                acceptIncoming(endpoint);
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
          RemovePendingConnection(endpoint);
          SetStatus(QStringLiteral("Connected to %1").arg(peer));
          LogLine(QStringLiteral("Connection accepted endpoint=%1 peer=%2")
                      .arg(endpoint, peer));
        },
        Qt::QueuedConnection);
  };

  listener.rejected_cb =
      [this](const std::string& endpoint_id, NearbyConnectionsQtFacade::Status status) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id), status]() {
              const QString peer = PeerLabelForEndpoint(endpoint);
              RemovePendingConnection(endpoint);
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
          RemovePendingConnection(endpoint);
          endpoint_peer_names_.remove(endpoint);
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
NearbyTrayController::BuildDiscoveryListener() {
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
          endpoint_peer_names_.remove(endpoint);
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
NearbyTrayController::BuildPayloadListener() {
  NearbyConnectionsQtFacade::PayloadListener listener;
  listener.payload_cb =
      [this](const std::string& endpoint_id,
             NearbyConnectionsQtFacade::Payload payload) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id),
             payload = std::move(payload)]() {
              if (payload.type == NearbyConnectionsQtFacade::Payload::Type::kBytes) {
                const auto& bytes = payload.bytes;
                const QString text = QString::fromUtf8(
                    reinterpret_cast<const char*>(bytes.data()),
                    static_cast<int>(bytes.size()));
                
                // Check if this is a FILE: metadata message (arrives before file)
                if (text.startsWith(QStringLiteral("FILE:"))) {
                  QString filename = text.mid(5).trimmed();
                  if (!filename.isEmpty()) {
                    pending_file_names_[endpoint] = filename;
                    LogLine(QStringLiteral("Received file metadata endpoint=%1 name=%2")
                                .arg(endpoint, filename));
                  }
                }
                
                emit payloadReceived(endpoint, QStringLiteral("bytes"), text);
                LogLine(QStringLiteral("Received bytes payload endpoint=%1 id=%2 size=%3")
                            .arg(endpoint)
                            .arg(payload.id)
                            .arg(bytes.size()));
              } else if (payload.type ==
                         NearbyConnectionsQtFacade::Payload::Type::kFile) {
                const QString path = QString::fromStdString(payload.file_path);
                QString file_name =
                    QString::fromStdString(payload.file_name);
                
                // Use pending filename if available
                if (pending_file_names_.contains(endpoint)) {
                  file_name = pending_file_names_.take(endpoint);
                }
                
                const QString final_path =
                    FinalizeReceivedFilePath(path, file_name, payload.id);
                emit payloadReceived(endpoint, QStringLiteral("file"),
                                     final_path);
                LogLine(QStringLiteral(
                            "Received file payload endpoint=%1 id=%2 path=%3 saved=%4 name=%5")
                            .arg(endpoint)
                            .arg(payload.id)
                            .arg(path, final_path, file_name));
              }
            },
            Qt::QueuedConnection);
      };

  listener.payload_progress_cb =
      [this](const std::string& endpoint_id,
             const NearbyConnectionsQtFacade::PayloadTransferUpdate& update) {
        QMetaObject::invokeMethod(
            this,
            [this, endpoint = QString::fromStdString(endpoint_id),
             update]() {
              QString direction = QStringLiteral("incoming");
              if (transfer_row_for_payload_.contains(update.payload_id)) {
                const int row = transfer_row_for_payload_.value(update.payload_id);
                if (row >= 0 && row < transfers_.size()) {
                  const QVariantMap transfer = transfers_[row].toMap();
                  const QString existing_direction =
                      transfer.value(QStringLiteral("direction")).toString();
                  if (!existing_direction.isEmpty()) {
                    direction = existing_direction;
                  }
                }
              }
              UpsertTransfer(endpoint, update.payload_id,
                             PayloadStatusToString(update.status),
                             update.bytes_transferred, update.total_bytes,
                             direction);
            },
            Qt::QueuedConnection);
      };
  return listener;
}

NearbyConnectionsQtFacade::MediumSelection
NearbyTrayController::BuildMediumSelection() const {
  NearbyConnectionsQtFacade::MediumSelection selection;
  selection.bluetooth = bluetooth_enabled_;
  selection.ble = ble_enabled_;
  selection.wifi_lan = wifi_lan_enabled_;
  selection.wifi_hotspot = wifi_hotspot_enabled_;
  selection.web_rtc = web_rtc_enabled_;
  return selection;
}

NearbyConnectionsQtFacade::AdvertisingOptions
NearbyTrayController::BuildAdvertisingOptions() const {
  NearbyConnectionsQtFacade::AdvertisingOptions options;
  options.strategy = StrategyFromName(connection_strategy_);
  options.allowed_mediums = BuildMediumSelection();
  options.auto_upgrade_bandwidth = true;
  options.enable_bluetooth_listening = true;
  options.enforce_topology_constraints = true;
  return options;
}

NearbyConnectionsQtFacade::DiscoveryOptions NearbyTrayController::BuildDiscoveryOptions()
    const {
  NearbyConnectionsQtFacade::DiscoveryOptions options;
  options.strategy = StrategyFromName(connection_strategy_);
  options.allowed_mediums = BuildMediumSelection();
  return options;
}

NearbyConnectionsQtFacade::ConnectionOptions NearbyTrayController::BuildConnectionOptions()
    const {
  NearbyConnectionsQtFacade::ConnectionOptions options;
  options.allowed_mediums = BuildMediumSelection();
  options.non_disruptive_hotspot_mode = true;
  return options;
}

void NearbyTrayController::AddDiscoveredDevice(const QString& endpoint_id) {
  if (discovered_devices_.contains(endpoint_id)) {
    return;
  }
  discovered_devices_.append(endpoint_id);
  emit discoveredDevicesChanged();
}

void NearbyTrayController::RemoveDiscoveredDevice(const QString& endpoint_id) {
  if (!discovered_devices_.removeOne(endpoint_id)) {
    return;
  }
  emit discoveredDevicesChanged();
}

void NearbyTrayController::AddConnectedDevice(const QString& endpoint_id) {
  if (connected_devices_.contains(endpoint_id)) {
    return;
  }
  connected_devices_.append(endpoint_id);
  emit connectedDevicesChanged();
}

void NearbyTrayController::RemoveConnectedDevice(const QString& endpoint_id) {
  if (!connected_devices_.removeOne(endpoint_id)) {
    return;
  }
  emit connectedDevicesChanged();
}

void NearbyTrayController::AddPendingConnection(const QString& endpoint_id) {
  if (pending_connections_.contains(endpoint_id)) {
    return;
  }
  pending_connections_.append(endpoint_id);
  emit pendingConnectionsChanged();
}

void NearbyTrayController::RemovePendingConnection(const QString& endpoint_id) {
  if (!pending_connections_.removeOne(endpoint_id)) {
    return;
  }
  emit pendingConnectionsChanged();
}

void NearbyTrayController::SetPeerNameForEndpoint(const QString& endpoint_id,
                                                  const QString& peer_name) {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return;
  }
  const QString previous = endpoint_peer_names_.value(endpoint).trimmed();
  const QString trimmed_name = peer_name.trimmed();
  if (trimmed_name.isEmpty()) {
    if (previous.isEmpty()) {
      return;
    }
    endpoint_peer_names_.remove(endpoint);
    emit discoveredDevicesChanged();
    emit connectedDevicesChanged();
    emit pendingConnectionsChanged();
    return;
  }
  if (previous == trimmed_name) {
    return;
  }
  endpoint_peer_names_[endpoint] = trimmed_name;
  emit discoveredDevicesChanged();
  emit connectedDevicesChanged();
  emit pendingConnectionsChanged();
}

QString NearbyTrayController::PeerLabelForEndpoint(
    const QString& endpoint_id) const {
  const QString endpoint = endpoint_id.trimmed();
  if (endpoint.isEmpty()) {
    return QStringLiteral("Unknown device");
  }
  const QString peer_name = endpoint_peer_names_.value(endpoint).trimmed();
  return peer_name.isEmpty() ? QStringLiteral("Unknown device") : peer_name;
}

QString NearbyTrayController::FinalizeReceivedFilePath(
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
            : QStringLiteral("%1_%2.%3").arg(stem).arg(suffix_index).arg(
                  suffix);
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

void NearbyTrayController::UpsertTransfer(const QString& endpoint_id,
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

  QVariantMap transfer{
      {QStringLiteral("payloadId"), payload_id},
      {QStringLiteral("endpointId"), endpoint_id},
      {QStringLiteral("status"), status},
      {QStringLiteral("bytesTransferred"),
       static_cast<qulonglong>(bytes_transferred)},
      {QStringLiteral("totalBytes"), static_cast<qulonglong>(total_bytes)},
      {QStringLiteral("progress"), progress},
      {QStringLiteral("medium"), medium},
      {QStringLiteral("direction"), direction},
  };

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

void NearbyTrayController::UpdateTransferMediumForEndpoint(
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

void NearbyTrayController::SetStatus(const QString& status) {
  if (status == status_message_) {
    return;
  }
  status_message_ = status;
  emit statusMessageChanged();
  LogLine(QStringLiteral("Status: %1").arg(status_message_));
}

void NearbyTrayController::LogLine(const QString& line) {
  if (!log_file_.isOpen()) {
    ReopenLogFile();
  }
  if (!log_file_.isOpen()) {
    return;
  }
  QTextStream stream(&log_file_);
  stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
         << " " << line << "\n";
  stream.flush();
}

void NearbyTrayController::ReopenLogFile() {
  if (log_file_.isOpen()) {
    log_file_.close();
  }
  log_file_.setFileName(log_path_);
  log_file_.open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly);
}

QString NearbyTrayController::StatusToString(NearbyConnectionsQtFacade::Status status) {
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

QString NearbyTrayController::PayloadStatusToString(
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

QString NearbyTrayController::MediumToString(NearbyConnectionsQtFacade::Medium medium) {
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
