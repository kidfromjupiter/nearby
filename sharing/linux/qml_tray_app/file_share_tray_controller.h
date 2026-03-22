#ifndef SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
#define SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_

#include <QObject>
#include <memory>

#include "file_share_state.h"
#include <sharing/linux/nearby_sharing_api.h>

using NearbySharingApi = nearby::sharing::NearbySharingApi;

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

  // Property accessors
  QString mode() const { return state_.mode(); }
  QString deviceName() const { return state_.deviceName(); }
  QString statusMessage() const { return state_.statusMessage(); }
  bool running() const { return state_.running(); }
  QString pendingSendFileName() const { return state_.pendingSendFileName(); }
  QString pendingSendFilePath() const { return state_.pendingSendFilePath(); }
  QVariantList discoveredTargets() const { return state_.discoveredTargets(); }
  QVariantList transfers() const { return state_.transfers(); }
  bool autoAcceptIncoming() const { return state_.autoAcceptIncoming(); }
  QString qrCodeUrl() const { return state_.qrCodeUrl(); }
  QStringList qrCodeRows() const { return state_.qrCodeRows(); }
  int qrCodeSize() const { return state_.qrCodeSize(); }
  QString logPath() const { return state_.logPath(); }

  // Public methods
  void setDeviceName(const QString& device_name);
  void setAutoAcceptIncoming(bool enabled);
  void setLogPath(const QString& path);

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void switchToReceiveMode();
  Q_INVOKABLE void switchToSendModeWithFile(const QString& file_path);
  Q_INVOKABLE void sendPendingFileToTarget(qlonglong share_target_id);
  Q_INVOKABLE void copyTextToClipboard(const QString& text);
  Q_INVOKABLE void openFileLocation(const QString& file_path);
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
  void initializeService();
  void attachServiceListeners();
  void loadSettings();
  void saveSettings() const;
  void updateQrCodeData();

  void startSendMode();
  void startReceiveMode();
  
  void updateTargetFromInfo(const NearbySharingApi::ShareTargetInfo& info);
  void handleTransferUpdate(const NearbySharingApi::TransferUpdateInfo& update);
  void handleTransferComplete(const NearbySharingApi::TransferUpdateInfo& update);
  void handleIncomingTransferComplete(const NearbySharingApi::TransferUpdateInfo& update,
                                      const QString& name, bool success);
  void handleOutgoingTransferComplete(const NearbySharingApi::TransferUpdateInfo& update,
                                      const QString& name, bool success);
  
  void setStatus(const QString& status);
  void notifyStateChange(const QString& property);

  std::unique_ptr<NearbySharingApi> service_;
  FileShareState state_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_FILE_SHARE_TRAY_CONTROLLER_H_
