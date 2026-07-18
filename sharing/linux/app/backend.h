#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include "QtQmlIntegration/qqmlintegration.h"
#include "qtmetamacros.h"
#include "sharing/attachment_container.h"
#include "sharing/linux/nearby_noop_analytics_recorder.h"
#include "sharing/linux/platform/linux_sharing_platform.h"
#include "sharing/nearby_sharing_service.h"
#include "sharing/share_target.h"
#include "sharing/share_target_discovered_callback.h"
#include "sharing/transfer_metadata.h"
#include "sharing/transfer_update_callback.h"

class ShareTargetModel : public QAbstractListModel {
  Q_OBJECT

 public:
  using ShareTarget = nearby::sharing::ShareTarget;

  enum Role {
    IdRole = Qt::UserRole + 1,
    DeviceNameRole,
    TypeRole,
  };

  explicit ShareTargetModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void ApplyTarget(const ShareTarget& target);
  void RemoveTarget(int64_t target_id);
  void Clear();
  std::optional<ShareTarget> FindTarget(int64_t target_id) const;

 private:
  int IndexOf(int64_t target_id) const;

  std::vector<ShareTarget> targets_;
};

class ShareTransferModel : public QAbstractListModel {
  Q_OBJECT

 public:
  using ShareTarget = nearby::sharing::ShareTarget;
  using TransferMetadata = nearby::sharing::TransferMetadata;

  enum Role {
    IdRole = Qt::UserRole + 1,
    DirectionRole,
    DeviceNameRole,
    StatusRole,
    ProgressRole,
    TransferredBytesRole,
    TotalBytesRole,
    TotalAttachmentsCountRole,
    TransferredAttachmentsCountRole,
    IsFinalStatusRole,
    AwaitingLocalConfirmationRole,
    LocalPathRole,
  };

  explicit ShareTransferModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void ApplyTarget(const ShareTarget& target);
  void ApplyTransfer(bool receive_mode, const ShareTarget& target,
                     const TransferMetadata& transfer, int64_t total_bytes);
  void PrepareOutgoingTransfer(int64_t target_id, const QString& local_path,
                               const std::optional<ShareTarget>& target);

 private:
  struct Row {
    int64_t id = 0;
    bool receive_mode = false;
    std::optional<ShareTarget> target;
    std::optional<TransferMetadata> transfer;
    int64_t total_bytes = 0;
    QString local_path;
  };

  int IndexOf(int64_t target_id) const;
  QVariant DataForRow(const Row& row, int role) const;
  void EmitRowChanged(int row);

  std::vector<Row> transfers_;
};

class Backend : public QObject,
                public nearby::sharing::ShareTargetDiscoveredCallback {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QAbstractListModel* targets READ targets CONSTANT)
  Q_PROPERTY(QAbstractListModel* transfers READ transfers CONSTANT)

 public:
  explicit Backend(QObject* parent = nullptr);
  ~Backend() override;

  QAbstractListModel* targets() { return &targets_; }
  QAbstractListModel* transfers() { return &transfers_; }

  Q_INVOKABLE void startReceive();
  Q_INVOKABLE void startDiscovery();
  Q_INVOKABLE void sendFile(qint64 share_target_id, const QString& path);
  Q_INVOKABLE void accept(qint64 share_target_id);
  Q_INVOKABLE void reject(qint64 share_target_id);
  Q_INVOKABLE void cancel(qint64 share_target_id);
  void shutdown();

 signals:
  void incomingTransfer(qint64 share_target_id);

 private:
  using NearbySharingService = nearby::sharing::NearbySharingService;
  using ShareTarget = nearby::sharing::ShareTarget;
  using TransferMetadata = nearby::sharing::TransferMetadata;

  class TransferCallback final
      : public nearby::sharing::TransferUpdateCallback {
   public:
    TransferCallback(Backend& backend, bool receive_mode)
        : backend_(backend), receive_mode_(receive_mode) {}

    void OnTransferUpdate(
        const ShareTarget& share_target,
        const nearby::sharing::AttachmentContainer& attachment_container,
        const TransferMetadata& transfer_metadata) override;

   private:
    Backend& backend_;
    bool receive_mode_;
  };

  enum class Mode {
    kNone,
    kReceive,
    kDiscovery,
  };

  void OnShareTargetDiscovered(const ShareTarget& target) override;
  void OnShareTargetUpdated(const ShareTarget& target) override;
  void OnShareTargetLost(const ShareTarget& target) override;
  void OnTransferUpdate(bool receive_mode, const ShareTarget& target,
                        const nearby::sharing::AttachmentContainer& attachments,
                        const TransferMetadata& transfer);

  void SetDesiredMode(Mode mode);
  void DriveMode();
  void OnModeStopped(Mode mode, NearbySharingService::StatusCodes status);
  void OnModeStarted(Mode mode, NearbySharingService::StatusCodes status);
  std::function<void(NearbySharingService::StatusCodes)> StatusCallback(
      QString operation);
  void ReportStatus(const QString& operation,
                    NearbySharingService::StatusCodes status);

  ShareTargetModel targets_;
  ShareTransferModel transfers_;
  nearby::sharing::linux::NoOpAnalyticsRecorder analytics_recorder_;
  nearby::sharing::linux::LinuxSharingPlatform platform_;
  NearbySharingService* service_ = nullptr;
  TransferCallback send_transfer_callback_;
  TransferCallback receive_transfer_callback_;
  Mode active_mode_ = Mode::kNone;
  Mode desired_mode_ = Mode::kReceive;
  bool initialized_ = false;
  bool mode_operation_in_flight_ = false;
  bool shutting_down_ = false;
};
