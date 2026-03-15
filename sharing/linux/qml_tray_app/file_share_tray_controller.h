#ifndef SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
#define SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_

#include <QObject>

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <memory>

#include "sharing/linux/nearby_sharing_api.h"

using NearbySharingApi = nearby::sharing::linux::NearbySharingApi;

class FileShareTrayController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
  Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString pendingSendFileName READ pendingSendFileName NOTIFY pendingSendFileNameChanged)
  Q_PROPERTY(QString pendingSendFilePath READ pendingSendFilePath NOTIFY pendingSendFilePathChanged)
  Q_PROPERTY(QVariantList discoveredTargets READ discoveredTargets NOTIFY discoveredTargetsChanged)
  Q_PROPERTY(QVariantList transfers READ transfers NOTIFY transfersChanged)
  Q_PROPERTY(bool autoAcceptIncoming READ autoAcceptIncoming WRITE setAutoAcceptIncoming NOTIFY autoAcceptIncomingChanged)
  Q_PROPERTY(QString qrCodeUrl READ qrCodeUrl NOTIFY qrCodeUrlChanged)
  Q_PROPERTY(QStringList qrCodeRows READ qrCodeRows NOTIFY qrCodeChanged)
  Q_PROPERTY(int qrCodeSize READ qrCodeSize NOTIFY qrCodeChanged)
  Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY logPathChanged)

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

  QVariantList discoveredTargets() const { return discovered_targets_; }
  QVariantList transfers() const { return transfers_; }

  bool autoAcceptIncoming() const { return auto_accept_incoming_; }
  void setAutoAcceptIncoming(bool enabled);

  QString qrCodeUrl() const { return qr_code_url_; }
  QStringList qrCodeRows() const { return qr_code_rows_; }
  int qrCodeSize() const { return qr_code_size_; }

  QString logPath() const { return log_path_; }
  void setLogPath(const QString& path);

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void switchToReceiveMode();
  Q_INVOKABLE void switchToSendModeWithFile(const QString& file_path);
  Q_INVOKABLE void sendPendingFileToTarget(qlonglong share_target_id);
  Q_INVOKABLE void copyTextToClipboard(const QString& text);
  Q_INVOKABLE void clearTransfers();
  Q_INVOKABLE void hideToTray();

 signals:
  void modeChanged();
  void deviceNameChanged();
  void statusMessageChanged();
  void runningChanged();
  void pendingSendFileNameChanged();
  void pendingSendFilePathChanged();
  void discoveredTargetsChanged();
  void transfersChanged();
  void autoAcceptIncomingChanged();
  void qrCodeUrlChanged();
  void qrCodeChanged();
  void logPathChanged();

  void requestTrayMessage(const QString& title, const QString& body);
  void requestCopyLinkTrayMessage(const QString& title, const QString& body,
                                  const QString& link);

 private:
  void CreateService();
  void AttachServiceListeners();

  void startSendMode();
  void startReceiveMode();
  void LoadSettings();
  void SaveSettings() const;
  void UpdateQrCodeData();

  void UpsertTarget(qlonglong share_target_id, const QString& name,
                    bool is_incoming);
  void RemoveTarget(qlonglong share_target_id);
  QString TargetName(qlonglong share_target_id) const;
  bool HasActiveTransferForTarget(qlonglong share_target_id) const;

  void UpsertTransfer(qlonglong share_target_id, const QString& target_name,
                      const QString& status, double progress,
                      qulonglong transferred_bytes, const QString& direction,
                      const QString& file_name);

  void SetStatus(const QString& status);
  bool HasActiveTransfers() const;

  static bool IsActiveTransferStatus(const QString& status);
  static QString StatusToString(NearbySharingApi::StatusCode status);
  static QString TransferStatusToString(NearbySharingApi::TransferStatus status);
  static bool IsFinalTransferStatus(NearbySharingApi::TransferStatus status);

  std::unique_ptr<NearbySharingApi> service_;

  QString mode_ = QStringLiteral("Receive");
  QString device_name_ = QStringLiteral("NearbyLinux");
  QString status_message_ = QStringLiteral("Idle");
  bool running_ = false;

  bool auto_accept_incoming_ = true;
  QString qr_code_url_;
  QStringList qr_code_rows_;
  int qr_code_size_ = 0;
  QString log_path_ = QStringLiteral("/tmp/nearby_qml_file_tray.log");

  QString pending_send_file_path_;
  QString pending_send_file_name_;
  qlonglong pending_send_target_id_ = 0;

  QVariantList discovered_targets_;
  QHash<qlonglong, int> discovered_row_by_target_;
  QSet<qlonglong> pending_target_removals_;
  QHash<qlonglong, QString> target_names_;

  QVariantList transfers_;
  QHash<qlonglong, int> transfer_row_by_target_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
