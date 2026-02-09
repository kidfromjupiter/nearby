#ifndef SHARING_LINUX_QML_TRAY_APP_NEARBY_TRAY_CONTROLLER_H_
#define SHARING_LINUX_QML_TRAY_APP_NEARBY_TRAY_CONTROLLER_H_

#include <QObject>

#include <QFile>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <string>
#include <vector>

#include "sharing/linux/nearby_connections_qt_facade.h"

class NearbyTrayController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)
  Q_PROPERTY(QString serviceId READ serviceId WRITE setServiceId NOTIFY serviceIdChanged)
  Q_PROPERTY(QString mediumsMode READ mediumsMode WRITE setMediumsMode NOTIFY mediumsModeChanged)
  Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothEnabledChanged)
  Q_PROPERTY(bool bleEnabled READ bleEnabled WRITE setBleEnabled NOTIFY bleEnabledChanged)
  Q_PROPERTY(bool wifiLanEnabled READ wifiLanEnabled WRITE setWifiLanEnabled NOTIFY wifiLanEnabledChanged)
  Q_PROPERTY(bool wifiHotspotEnabled READ wifiHotspotEnabled WRITE setWifiHotspotEnabled NOTIFY wifiHotspotEnabledChanged)
  Q_PROPERTY(bool webRtcEnabled READ webRtcEnabled WRITE setWebRtcEnabled NOTIFY webRtcEnabledChanged)
  Q_PROPERTY(bool autoAcceptIncoming READ autoAcceptIncoming WRITE setAutoAcceptIncoming NOTIFY autoAcceptIncomingChanged)
  Q_PROPERTY(QString connectionStrategy READ connectionStrategy WRITE setConnectionStrategy NOTIFY connectionStrategyChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY logPathChanged)
  Q_PROPERTY(QStringList discoveredDevices READ discoveredDevices NOTIFY discoveredDevicesChanged)
  Q_PROPERTY(QStringList connectedDevices READ connectedDevices NOTIFY connectedDevicesChanged)
  Q_PROPERTY(QStringList pendingConnections READ pendingConnections NOTIFY pendingConnectionsChanged)
  Q_PROPERTY(QVariantMap endpointMediums READ endpointMediums NOTIFY endpointMediumsChanged)
  Q_PROPERTY(QVariantList transfers READ transfers NOTIFY transfersChanged)

 public:
  explicit NearbyTrayController(QObject* parent = nullptr);
  ~NearbyTrayController() override;

  QString mode() const { return mode_; }
  void setMode(const QString& mode);

  QString deviceName() const { return device_name_; }
  void setDeviceName(const QString& device_name);

  QString serviceId() const { return QString::fromStdString(service_id_); }
  void setServiceId(const QString& service_id);

  QString mediumsMode() const { return mediums_mode_; }
  void setMediumsMode(const QString& mode);

  bool bluetoothEnabled() const { return bluetooth_enabled_; }
  void setBluetoothEnabled(bool enabled);

  bool bleEnabled() const { return ble_enabled_; }
  void setBleEnabled(bool enabled);

  bool wifiLanEnabled() const { return wifi_lan_enabled_; }
  void setWifiLanEnabled(bool enabled);

  bool wifiHotspotEnabled() const { return wifi_hotspot_enabled_; }
  void setWifiHotspotEnabled(bool enabled);

  bool webRtcEnabled() const { return web_rtc_enabled_; }
  void setWebRtcEnabled(bool enabled);

  bool autoAcceptIncoming() const { return auto_accept_incoming_; }
  void setAutoAcceptIncoming(bool enabled);

  QString connectionStrategy() const { return connection_strategy_; }
  void setConnectionStrategy(const QString& strategy);

  QString statusMessage() const { return status_message_; }
  bool running() const { return running_; }

  QString logPath() const { return log_path_; }
  void setLogPath(const QString& path);

  QStringList discoveredDevices() const { return discovered_devices_; }
  QStringList connectedDevices() const { return connected_devices_; }
  QStringList pendingConnections() const { return pending_connections_; }
  QVariantMap endpointMediums() const { return endpoint_mediums_; }
  QVariantList transfers() const { return transfers_; }

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void connectToDevice(const QString& endpoint_id);
  Q_INVOKABLE void disconnectDevice(const QString& endpoint_id);
  Q_INVOKABLE void acceptIncoming(const QString& endpoint_id);
  Q_INVOKABLE void rejectIncoming(const QString& endpoint_id);
  Q_INVOKABLE void sendText(const QString& endpoint_id, const QString& text);
  Q_INVOKABLE void initiateBandwidthUpgrade(const QString& endpoint_id);
  Q_INVOKABLE QString mediumForEndpoint(const QString& endpoint_id) const;
  Q_INVOKABLE QString peerNameForEndpoint(const QString& endpoint_id) const;
  Q_INVOKABLE void clearTransfers();
  Q_INVOKABLE void hideToTray();

 signals:
  void modeChanged();
  void deviceNameChanged();
  void serviceIdChanged();
  void mediumsModeChanged();
  void bluetoothEnabledChanged();
  void bleEnabledChanged();
  void wifiLanEnabledChanged();
  void wifiHotspotEnabledChanged();
  void webRtcEnabledChanged();
  void autoAcceptIncomingChanged();
  void connectionStrategyChanged();
  void statusMessageChanged();
  void runningChanged();
  void logPathChanged();
  void discoveredDevicesChanged();
  void connectedDevicesChanged();
  void pendingConnectionsChanged();
  void endpointMediumsChanged();
  void transfersChanged();

  void payloadReceived(const QString& endpoint_id, const QString& type,
                       const QString& value);
  void requestTrayMessage(const QString& title, const QString& body);

 private:
  void startSendMode();
  void startReceiveMode();
  void LoadSettings();
  void SaveSettings() const;

  std::vector<uint8_t> BuildEndpointInfo() const;

  nearby::sharing::linux::NearbyConnectionsQtFacade::ConnectionListener
  BuildConnectionListener();
  nearby::sharing::linux::NearbyConnectionsQtFacade::DiscoveryListener
  BuildDiscoveryListener();
  nearby::sharing::linux::NearbyConnectionsQtFacade::PayloadListener
  BuildPayloadListener();

  nearby::sharing::linux::NearbyConnectionsQtFacade::AdvertisingOptions
  BuildAdvertisingOptions() const;
  nearby::sharing::linux::NearbyConnectionsQtFacade::DiscoveryOptions
  BuildDiscoveryOptions() const;
  nearby::sharing::linux::NearbyConnectionsQtFacade::ConnectionOptions
  BuildConnectionOptions() const;
  nearby::sharing::linux::NearbyConnectionsQtFacade::MediumSelection
  BuildMediumSelection() const;

  void AddDiscoveredDevice(const QString& endpoint_id);
  void RemoveDiscoveredDevice(const QString& endpoint_id);
  void AddConnectedDevice(const QString& endpoint_id);
  void RemoveConnectedDevice(const QString& endpoint_id);
  void AddPendingConnection(const QString& endpoint_id);
  void RemovePendingConnection(const QString& endpoint_id);
  void SetPeerNameForEndpoint(const QString& endpoint_id,
                              const QString& peer_name);
  QString PeerLabelForEndpoint(const QString& endpoint_id) const;
  QString FinalizeReceivedFilePath(const QString& received_path,
                                   const QString& received_file_name,
                                   qlonglong payload_id) const;

  void UpsertTransfer(const QString& endpoint_id, qlonglong payload_id,
                      const QString& status, qulonglong bytes_transferred,
                      qulonglong total_bytes, const QString& direction);
  void UpdateTransferMediumForEndpoint(const QString& endpoint_id,
                                       const QString& medium);

  void SetStatus(const QString& status);
  void LogLine(const QString& line);
  void ReopenLogFile();

  static QString StatusToString(
      nearby::sharing::linux::NearbyConnectionsQtFacade::Status status);
  static QString PayloadStatusToString(
      nearby::sharing::linux::NearbyConnectionsQtFacade::PayloadStatus status);
  static QString MediumToString(
      nearby::sharing::linux::NearbyConnectionsQtFacade::Medium medium);

  nearby::sharing::linux::NearbyConnectionsQtFacade service_;

  QString mode_ = QStringLiteral("Receive");
  QString device_name_ = QStringLiteral("NearbyQt");
  std::string service_id_ = "com.nearby.qml.tray";
  QString mediums_mode_ = QStringLiteral("balanced");
  QString connection_strategy_ = QStringLiteral("P2pCluster");
  QString status_message_ = QStringLiteral("Idle");
  bool running_ = false;

  bool bluetooth_enabled_ = true;
  bool ble_enabled_ = true;
  bool wifi_lan_enabled_ = true;
  bool wifi_hotspot_enabled_ = true;
  bool web_rtc_enabled_ = false;
  bool auto_accept_incoming_ = false;

  QString log_path_ = QStringLiteral("/tmp/nearby_qml_tray.log");
  QFile log_file_;

  QStringList discovered_devices_;
  QStringList connected_devices_;
  QStringList pending_connections_;
  QHash<QString, QString> endpoint_peer_names_;
  QVariantMap endpoint_mediums_;
  QVariantList transfers_;
  QHash<qlonglong, int> transfer_row_for_payload_;
  QHash<QString, QString> pending_file_names_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_NEARBY_TRAY_CONTROLLER_H_
