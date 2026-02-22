#ifndef SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
#define SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_

#include <QObject>

#include <QFile>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <string>
#include <vector>

#include "sharing/linux/nearby_connections_qt_facade.h"

using NearbyConnectionsQtFacade = nearby::sharing::NearbyConnectionsQtFacade;

class FileShareTrayController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
  Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString pendingSendFileName READ pendingSendFileName NOTIFY pendingSendFileNameChanged)
  Q_PROPERTY(QString pendingSendFilePath READ pendingSendFilePath NOTIFY pendingSendFilePathChanged)
  Q_PROPERTY(QStringList discoveredDevices READ discoveredDevices NOTIFY discoveredDevicesChanged)
  Q_PROPERTY(QStringList connectedDevices READ connectedDevices NOTIFY connectedDevicesChanged)
  Q_PROPERTY(QVariantMap endpointMediums READ endpointMediums NOTIFY endpointMediumsChanged)
  Q_PROPERTY(QVariantList transfers READ transfers NOTIFY transfersChanged)

 public:
  explicit FileShareTrayController(QObject* parent = nullptr);
  ~FileShareTrayController() override;

  QString mode() const { return mode_; }

  QString deviceName() const { return device_name_; }
  void setDeviceName(const QString& device_name);

  QString statusMessage() const { return status_message_; }
  bool running() const { return running_; }

  QString pendingSendFileName() const { return pending_send_file_name_; }
  QString pendingSendFilePath() const { return pending_send_file_path_; }

  QStringList discoveredDevices() const { return discovered_devices_; }
  QStringList connectedDevices() const { return connected_devices_; }
  QVariantMap endpointMediums() const { return endpoint_mediums_; }
  QVariantList transfers() const { return transfers_; }

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void switchToReceiveMode();
  Q_INVOKABLE void switchToSendModeWithFile(const QString& file_path);
  Q_INVOKABLE void sendPendingFileToEndpoint(const QString& endpoint_id);
  Q_INVOKABLE QString mediumForEndpoint(const QString& endpoint_id) const;
  Q_INVOKABLE QString peerNameForEndpoint(const QString& endpoint_id) const;
  Q_INVOKABLE void clearTransfers();
  Q_INVOKABLE void hideToTray();

 signals:
  void modeChanged();
  void deviceNameChanged();
  void statusMessageChanged();
  void runningChanged();
  void pendingSendFileNameChanged();
  void pendingSendFilePathChanged();
  void discoveredDevicesChanged();
  void connectedDevicesChanged();
  void endpointMediumsChanged();
  void transfersChanged();

  void requestTrayMessage(const QString& title, const QString& body);

 private:
  void startSendMode();
  void startReceiveMode();
  void LoadSettings();
  void SaveSettings() const;

  std::vector<uint8_t> BuildEndpointInfo() const;

  NearbyConnectionsQtFacade::ConnectionListener BuildConnectionListener();
  NearbyConnectionsQtFacade::DiscoveryListener BuildDiscoveryListener();
  NearbyConnectionsQtFacade::PayloadListener BuildPayloadListener();

  NearbyConnectionsQtFacade::AdvertisingOptions BuildAdvertisingOptions() const;
  NearbyConnectionsQtFacade::DiscoveryOptions BuildDiscoveryOptions() const;
  NearbyConnectionsQtFacade::ConnectionOptions BuildConnectionOptions() const;
  NearbyConnectionsQtFacade::MediumSelection BuildMediumSelection() const;

  void acceptIncomingInternal(const QString& endpoint_id);
  void requestConnectionForSend(const QString& endpoint_id);
  void sendPendingFile(const QString& endpoint_id);
  void disconnectDevice(const QString& endpoint_id);

  void AddDiscoveredDevice(const QString& endpoint_id);
  void RemoveDiscoveredDevice(const QString& endpoint_id);
  void AddConnectedDevice(const QString& endpoint_id);
  void RemoveConnectedDevice(const QString& endpoint_id);
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
  bool HasActiveTransfers() const;
  void LogLine(const QString& line);
  void ReopenLogFile();

  static QString StatusToString(NearbyConnectionsQtFacade::Status status);
  static QString PayloadStatusToString(
      NearbyConnectionsQtFacade::PayloadStatus status);
  static QString MediumToString(NearbyConnectionsQtFacade::Medium medium);

  NearbyConnectionsQtFacade service_;

  QString mode_ = QStringLiteral("Receive");
  QString device_name_ = QStringLiteral("NearbyQtFile");
  const std::string service_id_ = "com.nearby.qml.tray";
  QString status_message_ = QStringLiteral("Idle");
  bool running_ = false;

  QString pending_send_file_path_;
  QString pending_send_file_name_;
  QString target_endpoint_for_send_;

  QStringList discovered_devices_;
  QStringList connected_devices_;
  QHash<QString, QString> endpoint_peer_names_;
  QVariantMap endpoint_mediums_;

  QVariantList transfers_;
  QHash<qlonglong, int> transfer_row_for_payload_;

  QHash<QString, QString> pending_file_names_;
  QHash<qlonglong, QString> incoming_file_paths_;
  QHash<qlonglong, QString> incoming_file_names_;
  QHash<qlonglong, QString> incoming_file_endpoints_;
  QHash<qlonglong, QString> outgoing_file_payload_to_endpoint_;
  QHash<qlonglong, QString> outgoing_file_payload_to_name_;
  QSet<qlonglong> send_terminal_notified_;

  QString log_path_ = QStringLiteral("/tmp/nearby_qml_file_tray.log");
  QFile log_file_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
