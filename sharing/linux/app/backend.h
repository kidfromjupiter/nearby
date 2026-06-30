#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
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
  std::optional<ShareTarget> FindTarget(int64_t target_id) const;

 private:
  int IndexOf(int64_t target_id) const;

  std::vector<ShareTarget> targets_;
};

class ShareTransferModel : public QAbstractListModel {
  Q_OBJECT

 public:
  using ShareTarget = nearby::sharing::linux::app::ShareTarget;
  using Transfer = nearby::sharing::linux::app::Transfer;

  enum Role {
    IdRole = Qt::UserRole + 1,
    DirectionRole,
    DeviceNameRole,
    TypeRole,
    StatusRole,
    ProgressRole,
    TransferredBytesRole,
    TotalBytesRole,
    TransferSpeedRole,
    EstimatedTimeRemainingRole,
    TotalAttachmentsCountRole,
    TransferredAttachmentsCountRole,
    IsFinalStatusRole,
    HasTargetRole,
    HasTransferRole,
    AwaitingLocalConfirmationRole,
    LocalPathRole,
  };

  explicit ShareTransferModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void ApplyTarget(const ShareTarget& target);
  void ApplyTransfer(const QString& direction, const ShareTarget& target,
                     const Transfer& transfer);
  void PrepareOutgoingTransfer(int64_t target_id, const QString& local_path,
                               const std::optional<ShareTarget>& target);
  void RemoveTransfer(int64_t target_id);

 private:
  struct Row {
    int64_t id = 0;
    QString direction;
    std::optional<ShareTarget> target;
    std::optional<Transfer> transfer;
    QString local_path;
  };

  int IndexOf(int64_t target_id) const;
  QVariant DataForRow(const Row& row, int role) const;
  void EmitRowChanged(int row);

  std::vector<Row> transfers_;
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
  Q_PROPERTY(QAbstractListModel* transfers READ transfers CONSTANT)

 public:
  explicit Backend(QObject* parent = nullptr);
  ~Backend() override;

  QString statusText() const { return status_text_; }
  bool receiveRegistered() const { return status_.receive_registered; }
  bool discoveryRegistered() const { return status_.discovery_registered; }
  bool scanning() const { return status_.is_scanning; }
  bool transferring() const { return status_.is_transferring; }
  QAbstractListModel* targets() { return &targets_; }
  QAbstractListModel* transfers() { return &transfers_; }

  Q_INVOKABLE void startReceive();
  Q_INVOKABLE void stopReceive();
  Q_INVOKABLE void startDiscovery();
  Q_INVOKABLE void stopDiscovery();
  Q_INVOKABLE void sendFile(qint64 share_target_id, const QString& path);
  Q_INVOKABLE void prepareOutgoingTransfer(qint64 share_target_id,
                                           const QString& path);
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
  ShareTransferModel transfers_;
  std::unique_ptr<Client> client_;
  bool is_incoming_transfer_ = false;
};
