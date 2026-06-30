#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include "QtQmlIntegration/qqmlintegration.h"
#include "qtmetamacros.h"
#include "sharing/linux/app/nearby_sharing_dbus_client.h"

class ShareTargetModel : public QAbstractListModel {
  Q_OBJECT

 public:
  using ShareTarget = nearby::sharing::linux::app::ShareTarget;

  enum Role {
    IdRole = Qt::UserRole + 1,
    DeviceNameRole,
    TypeRole,
    IsIncomingRole,
    IsKnownRole,
    DeviceIdRole,
    ForSelfShareRole,
    VendorIdRole,
    ReceiveDisabledRole,
  };

  explicit ShareTargetModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void ApplyTarget(const ShareTarget& target);
  void RemoveTarget(int64_t target_id);
  void ResetTargets(const std::vector<ShareTarget>& targets);

 private:
  int IndexOf(int64_t target_id) const;

  std::vector<ShareTarget> targets_;
};

class Backend
    : public QObject,
      public nearby::sharing::linux::app::NearbySharingDbusClient::Observer {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(bool receiveRegistered READ receiveRegistered NOTIFY statusChanged)
  Q_PROPERTY(
      bool discoveryRegistered READ discoveryRegistered NOTIFY statusChanged)
  Q_PROPERTY(bool scanning READ scanning NOTIFY statusChanged)
  Q_PROPERTY(bool transferring READ transferring NOTIFY statusChanged)
  Q_PROPERTY(QAbstractListModel* targets READ targets CONSTANT)

 public:
  explicit Backend(QObject* parent = nullptr);
  ~Backend() override;

  QString statusText() const { return status_text_; }
  bool receiveRegistered() const { return status_.receive_registered; }
  bool discoveryRegistered() const { return status_.discovery_registered; }
  bool scanning() const { return status_.is_scanning; }
  bool transferring() const { return status_.is_transferring; }
  QAbstractListModel* targets() { return &targets_; }

  Q_INVOKABLE void startReceive();
  Q_INVOKABLE void stopReceive();
  Q_INVOKABLE void startDiscovery();
  Q_INVOKABLE void stopDiscovery();
  Q_INVOKABLE void sendFile(qint64 share_target_id, const QString& path);
  Q_INVOKABLE void accept(qint64 share_target_id);
  Q_INVOKABLE void reject(qint64 share_target_id);
  Q_INVOKABLE void cancel(qint64 share_target_id);

 signals:
  void statusTextChanged();
  void statusChanged();
  void incomingTransfer(qint64 share_target_id, QString device_name,
                        QString status);
  void transferUpdate(qint64 share_target_id, QString device_name,
                      QString status, double progress);

 private:
  using Client = nearby::sharing::linux::app::NearbySharingDbusClient;
  using ShareTarget = nearby::sharing::linux::app::ShareTarget;
  using Status = nearby::sharing::linux::app::Status;
  using Transfer = nearby::sharing::linux::app::Transfer;

  void OnTargetDiscovered(const ShareTarget& target) override;
  void OnTargetUpdated(const ShareTarget& target) override;
  void OnTargetLost(const ShareTarget& target) override;
  void OnIncomingTransfer(const std::string& direction,
                          const ShareTarget& target,
                          const Transfer& transfer) override;
  void OnTransferUpdate(const std::string& direction, const ShareTarget& target,
                        const Transfer& transfer) override;
  void OnStatusChanged(const Status& status) override;

  void ApplyTarget(const ShareTarget& target);
  void RemoveTarget(const ShareTarget& target);
  void ApplyStatus(const Status& status);
  void ApplyCommandResult(const QString& command,
                          const std::tuple<bool, std::string>& result);
  void RunCommand(
      const QString& command,
      const std::function<std::tuple<bool, std::string>()>& operation);
  void SetStatusText(const QString& text);

  QString status_text_;
  Status status_;
  ShareTargetModel targets_;
  std::unique_ptr<Client> client_;
  bool is_incoming_transfer_ = false;
};
