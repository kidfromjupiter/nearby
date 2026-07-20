#include "sharing/linux/app/backend.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <utility>

#include <QDebug>
#include <QMetaObject>
#include <QPointer>
#include <QSysInfo>
#include <QUrl>

#include "absl/time/time.h"
#include "connections/implementation/flags/nearby_connections_feature_flags.h"
#include "internal/base/file_path.h"
#include "internal/flags/nearby_flags.h"
#include "sharing/advertisement.h"
#include "sharing/file_attachment.h"
#include "sharing/flags/generated/nearby_sharing_feature_flags.h"
#include "sharing/nearby_sharing_service_factory.h"
#include "sharing/nearby_sharing_settings.h"
#include "sharing/proto/enums.pb.h"

namespace {

using NearbySharingService = nearby::sharing::NearbySharingService;

constexpr std::chrono::seconds kShutdownTimeout(10);

QString ToQString(const std::string& value) {
  return QString::fromStdString(value);
}

QString NormalizeLocalPath(const QString& path) {
  const QUrl url(path);
  return url.isValid() && url.isLocalFile() ? url.toLocalFile() : path;
}

void ConfigureFlags() {
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::sharing::config_package_nearby::nearby_sharing_feature::
          kEnableBleForTransfer,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kEnableBleL2cap,
      true);
  nearby::NearbyFlags::GetInstance().OverrideBoolFlagValue(
      nearby::connections::config_package_nearby::nearby_connections_feature::
          kRefactorBleL2cap,
      true);
}

std::unique_ptr<nearby::sharing::AttachmentContainer> CreateFileAttachments(
    const std::string& path) {
  nearby::sharing::AttachmentContainer::Builder builder;
  builder.AddFileAttachment(
      nearby::sharing::FileAttachment(nearby::FilePath(path)));
  return builder.Build();
}

NearbySharingService::StatusCodes ShutdownService(
    NearbySharingService& service) {
  struct State {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<NearbySharingService::StatusCodes> status;
  };
  auto state = std::make_shared<State>();
  service.Shutdown([state](NearbySharingService::StatusCodes status) {
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      state->status = status;
    }
    state->cv.notify_one();
  });

  std::unique_lock<std::mutex> lock(state->mutex);
  if (!state->cv.wait_for(lock, kShutdownTimeout,
                          [&] { return state->status.has_value(); })) {
    return NearbySharingService::StatusCodes::kError;
  }
  return *state->status;
}

void PostStatus(Backend* backend, std::function<void()> callback) {
  QPointer<Backend> guarded_backend(backend);
  if (!guarded_backend) {
    return;
  }
  QMetaObject::invokeMethod(
      guarded_backend,
      [guarded_backend, callback = std::move(callback)]() mutable {
        if (guarded_backend) {
          callback();
        }
      },
      Qt::QueuedConnection);
}

}  // namespace

ShareTargetModel::ShareTargetModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ShareTargetModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(targets_.size());
}

QVariant ShareTargetModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(targets_.size())) {
    return {};
  }

  const ShareTarget& target = targets_[index.row()];
  switch (role) {
    case IdRole:
      return QVariant::fromValue<qint64>(target.id);
    case DeviceNameRole:
      return ToQString(target.device_name);
    case TypeRole:
      return static_cast<int>(target.type);
    default:
      return {};
  }
}

QHash<int, QByteArray> ShareTargetModel::roleNames() const {
  return {
      {IdRole, "targetId"},
      {DeviceNameRole, "deviceName"},
      {TypeRole, "type"},
  };
}

void ShareTargetModel::ApplyTarget(const ShareTarget& target) {
  const int row = IndexOf(target.id);
  if (row < 0) {
    const int insert_row = static_cast<int>(targets_.size());
    beginInsertRows(QModelIndex(), insert_row, insert_row);
    targets_.push_back(target);
    endInsertRows();
    return;
  }

  targets_[row] = target;
  emit dataChanged(index(row), index(row), {IdRole, DeviceNameRole, TypeRole});
}

void ShareTargetModel::RemoveTarget(int64_t target_id) {
  const int row = IndexOf(target_id);
  if (row < 0) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  targets_.erase(targets_.begin() + row);
  endRemoveRows();
}

void ShareTargetModel::Clear() {
  if (targets_.empty()) {
    return;
  }
  beginResetModel();
  targets_.clear();
  endResetModel();
}

std::optional<ShareTargetModel::ShareTarget> ShareTargetModel::FindTarget(
    int64_t target_id) const {
  const int row = IndexOf(target_id);
  return row < 0 ? std::nullopt : std::optional<ShareTarget>(targets_[row]);
}

int ShareTargetModel::IndexOf(int64_t target_id) const {
  for (int i = 0; i < static_cast<int>(targets_.size()); ++i) {
    if (targets_[i].id == target_id) {
      return i;
    }
  }
  return -1;
}

ShareTransferModel::ShareTransferModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ShareTransferModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(transfers_.size());
}

QVariant ShareTransferModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(transfers_.size())) {
    return {};
  }
  return DataForRow(transfers_[index.row()], role);
}

QHash<int, QByteArray> ShareTransferModel::roleNames() const {
  return {
      {IdRole, "transferId"},
      {DirectionRole, "direction"},
      {DeviceNameRole, "deviceName"},
      {StatusRole, "status"},
      {ProgressRole, "progress"},
      {TransferredBytesRole, "transferredBytes"},
      {TotalBytesRole, "totalBytes"},
      {TotalAttachmentsCountRole, "totalAttachmentsCount"},
      {TransferredAttachmentsCountRole, "transferredAttachmentsCount"},
      {IsFinalStatusRole, "isFinalStatus"},
      {AwaitingLocalConfirmationRole, "awaitingLocalConfirmation"},
      {LocalPathRole, "localPath"},
  };
}

void ShareTransferModel::ApplyTarget(const ShareTarget& target) {
  const int row = IndexOf(target.id);
  if (row < 0) {
    return;
  }
  transfers_[row].target = target;
  EmitRowChanged(row);
}

void ShareTransferModel::ApplyTransfer(bool receive_mode,
                                       const ShareTarget& target,
                                       const TransferMetadata& transfer,
                                       int64_t total_bytes) {
  int row = IndexOf(target.id);
  if (row < 0) {
    row = static_cast<int>(transfers_.size());
    beginInsertRows(QModelIndex(), row, row);
    transfers_.push_back(
        Row{target.id, receive_mode, target, transfer, total_bytes, {}});
    endInsertRows();
    return;
  }

  transfers_[row].receive_mode = receive_mode;
  transfers_[row].target = target;
  transfers_[row].transfer = transfer;
  transfers_[row].total_bytes = total_bytes;
  EmitRowChanged(row);
}

void ShareTransferModel::PrepareOutgoingTransfer(
    int64_t target_id, const QString& local_path,
    const std::optional<ShareTarget>& target) {
  int row = IndexOf(target_id);
  if (row < 0) {
    row = static_cast<int>(transfers_.size());
    beginInsertRows(QModelIndex(), row, row);
    Row transfer;
    transfer.id = target_id;
    transfer.target = target;
    transfer.local_path = local_path;
    transfers_.push_back(std::move(transfer));
    endInsertRows();
    return;
  }

  transfers_[row].receive_mode = false;
  transfers_[row].target = target;
  transfers_[row].transfer.reset();
  transfers_[row].total_bytes = 0;
  transfers_[row].local_path = local_path;
  EmitRowChanged(row);
}

int ShareTransferModel::IndexOf(int64_t target_id) const {
  for (int i = 0; i < static_cast<int>(transfers_.size()); ++i) {
    if (transfers_[i].id == target_id) {
      return i;
    }
  }
  return -1;
}

QVariant ShareTransferModel::DataForRow(const Row& row, int role) const {
  const auto& target = row.target;
  const auto& transfer = row.transfer;
  switch (role) {
    case IdRole:
      return QVariant::fromValue<qint64>(row.id);
    case DirectionRole:
      return row.receive_mode ? QStringLiteral("receive")
                              : QStringLiteral("send");
    case DeviceNameRole:
      return target.has_value() && !target->device_name.empty()
                 ? ToQString(target->device_name)
                 : QStringLiteral("Unknown device");
    case StatusRole:
      return transfer.has_value() ? ToQString(TransferMetadata::StatusToString(
                                        transfer->status()))
                                  : QString();
    case ProgressRole:
      return transfer.has_value()
                 ? std::clamp(static_cast<double>(transfer->progress()) / 100.0,
                              0.0, 1.0)
                 : 0.0;
    case TransferredBytesRole:
      return QVariant::fromValue<qulonglong>(
          transfer.has_value() ? transfer->transferred_bytes() : 0);
    case TotalBytesRole:
      return QVariant::fromValue<qint64>(row.total_bytes);
    case TotalAttachmentsCountRole:
      return transfer.has_value() ? transfer->total_attachments_count() : 0;
    case TransferredAttachmentsCountRole:
      return transfer.has_value() ? transfer->transferred_attachments_count()
                                  : 0;
    case IsFinalStatusRole:
      return transfer.has_value() && transfer->is_final_status();
    case AwaitingLocalConfirmationRole:
      return transfer.has_value() &&
             transfer->status() ==
                 TransferMetadata::Status::kAwaitingLocalConfirmation;
    case LocalPathRole:
      return row.local_path;
    default:
      return {};
  }
}

void ShareTransferModel::EmitRowChanged(int row) {
  emit dataChanged(
      index(row), index(row),
      {IdRole, DirectionRole, DeviceNameRole, StatusRole, ProgressRole,
       TransferredBytesRole, TotalBytesRole, TotalAttachmentsCountRole,
       TransferredAttachmentsCountRole, IsFinalStatusRole,
       AwaitingLocalConfirmationRole, LocalPathRole});
}

void Backend::TransferCallback::OnTransferUpdate(
    const ShareTarget& share_target,
    const nearby::sharing::AttachmentContainer& attachment_container,
    const TransferMetadata& transfer_metadata) {
  backend_.OnTransferUpdate(receive_mode_, share_target, attachment_container,
                            transfer_metadata);
}

Backend::Backend(QObject* parent)
    : QObject(parent),
      targets_(this),
      transfers_(this),
      send_transfer_callback_(*this, false),
      receive_transfer_callback_(*this, true) {
  ConfigureFlags();
  service_ = nearby::sharing::NearbySharingServiceFactory::GetInstance()
                 ->CreateSharingService(platform_, &analytics_recorder_,
                                        /*event_logger=*/nullptr,
                                        /*supports_file_sync=*/false);
  if (service_ == nullptr) {
    qCritical() << "Failed to create NearbySharingService";
    desired_mode_ = Mode::kNone;
    return;
  }

  service_->GetSettings()->SetDataUsage(
      nearby::sharing::proto::WIFI_ONLY_DATA_USAGE);
  QPointer<Backend> backend(this);
  service_->SetVisibility(
      nearby::sharing::proto::DEVICE_VISIBILITY_EVERYONE, absl::ZeroDuration(),
      [backend](NearbySharingService::StatusCodes status) mutable {
        if (!backend) {
          return;
        }
        PostStatus(backend, [backend, status]() {
          if (!backend || backend->shutting_down_) {
            return;
          }
          backend->initialized_ =
              status == NearbySharingService::StatusCodes::kOk;
          backend->ReportStatus(QStringLiteral("Initialize"), status);
          if (backend->initialized_) {
            backend->DriveMode();
          } else {
            backend->desired_mode_ = Mode::kNone;
          }
        });
      });
}

Backend::~Backend() {
  shutdown();
}

QString Backend::hostname() const {
  return QSysInfo::machineHostName();
}

void Backend::startReceive() {
  SetDesiredMode(Mode::kReceive);
}

void Backend::startDiscovery() {
  SetDesiredMode(Mode::kDiscovery);
}

bool Backend::sendFile(qint64 share_target_id, const QString& path) {
  if (service_ == nullptr || shutting_down_) {
    return false;
  }
  const QString local_path = NormalizeLocalPath(path);
  const std::string native_path = local_path.toStdString();
  std::error_code error;
  if (!std::filesystem::is_regular_file(native_path, error)) {
    qWarning() << "Send file failed: path is not a regular file" << local_path;
    return false;
  }
  const auto target = targets_.FindTarget(share_target_id);
  if (!target.has_value()) {
    qWarning() << "Send file failed: unknown target" << share_target_id;
    return false;
  }

  transfers_.PrepareOutgoingTransfer(share_target_id, local_path, target);
  QPointer<Backend> backend(this);
  service_->SendAttachments(
      share_target_id, CreateFileAttachments(native_path),
      [backend, share_target_id](NearbySharingService::StatusCodes status) {
        if (!backend) {
          return;
        }
        PostStatus(backend, [backend, share_target_id, status]() {
          if (!backend || backend->shutting_down_) {
            return;
          }
          backend->ReportStatus(QStringLiteral("Send file"), status);
          if (status != NearbySharingService::StatusCodes::kOk) {
            emit backend->outgoingTransferStartFailed(share_target_id);
          }
        });
      });
  return true;
}

void Backend::accept(qint64 share_target_id) {
  if (service_ != nullptr && !shutting_down_) {
    service_->Accept(share_target_id,
                     StatusCallback(QStringLiteral("Accept transfer")));
  }
}

void Backend::reject(qint64 share_target_id) {
  if (service_ != nullptr && !shutting_down_) {
    service_->Reject(share_target_id,
                     StatusCallback(QStringLiteral("Reject transfer")));
  }
}

void Backend::cancel(qint64 share_target_id) {
  if (service_ != nullptr && !shutting_down_) {
    service_->Cancel(share_target_id,
                     StatusCallback(QStringLiteral("Cancel transfer")));
  }
}

void Backend::shutdown() {
  if (service_ == nullptr || shutting_down_) {
    return;
  }
  shutting_down_ = true;
  desired_mode_ = Mode::kNone;
  const auto status = ShutdownService(*service_);
  if (status != NearbySharingService::StatusCodes::kOk) {
    qWarning() << "Nearby Sharing shutdown failed:"
               << NearbySharingService::StatusCodeToString(status);
  }
  service_ = nullptr;
}

void Backend::OnShareTargetDiscovered(const ShareTarget& target) {
  QPointer<Backend> backend(this);
  PostStatus(this, [backend, target]() {
    if (!backend || backend->shutting_down_) {
      return;
    }
    backend->targets_.ApplyTarget(target);
    backend->transfers_.ApplyTarget(target);
  });
}

void Backend::OnShareTargetUpdated(const ShareTarget& target) {
  OnShareTargetDiscovered(target);
}

void Backend::OnShareTargetLost(const ShareTarget& target) {
  QPointer<Backend> backend(this);
  PostStatus(this, [backend, id = target.id]() {
    if (backend && !backend->shutting_down_) {
      backend->targets_.RemoveTarget(id);
    }
  });
}

void Backend::OnTransferUpdate(
    bool receive_mode, const ShareTarget& target,
    const nearby::sharing::AttachmentContainer& attachments,
    const TransferMetadata& transfer) {
  const int64_t total_bytes = attachments.GetTotalAttachmentsSize();
  QPointer<Backend> backend(this);
  PostStatus(this, [backend, receive_mode, target, transfer, total_bytes]() {
    if (!backend || backend->shutting_down_) {
      return;
    }
    backend->targets_.ApplyTarget(target);
    backend->transfers_.ApplyTransfer(receive_mode, target, transfer,
                                      total_bytes);
    if (receive_mode &&
        transfer.status() ==
            TransferMetadata::Status::kAwaitingLocalConfirmation) {
      emit backend->incomingTransfer(target.id);
    }
  });
}

void Backend::SetDesiredMode(Mode mode) {
  desired_mode_ = mode;
  DriveMode();
}

void Backend::DriveMode() {
  if (!initialized_ || service_ == nullptr || shutting_down_ ||
      mode_operation_in_flight_ || active_mode_ == desired_mode_) {
    return;
  }

  mode_operation_in_flight_ = true;
  QPointer<Backend> backend(this);
  if (active_mode_ == Mode::kReceive) {
    service_->UnregisterReceiveSurface(
        &receive_transfer_callback_,
        [backend](NearbySharingService::StatusCodes status) mutable {
          if (backend) {
            PostStatus(backend, [backend, status]() {
              if (backend) {
                backend->OnModeStopped(Mode::kReceive, status);
              }
            });
          }
        });
    return;
  }
  if (active_mode_ == Mode::kDiscovery) {
    service_->UnregisterSendSurface(
        &send_transfer_callback_,
        [backend](NearbySharingService::StatusCodes status) mutable {
          if (backend) {
            PostStatus(backend, [backend, status]() {
              if (backend) {
                backend->OnModeStopped(Mode::kDiscovery, status);
              }
            });
          }
        });
    return;
  }

  const Mode mode = desired_mode_;
  if (mode == Mode::kReceive) {
    service_->RegisterReceiveSurface(
        &receive_transfer_callback_,
        NearbySharingService::ReceiveSurfaceState::kForeground,
        nearby::sharing::Advertisement::BlockedVendorId::kNone,
        [backend, mode](NearbySharingService::StatusCodes status) mutable {
          if (backend) {
            PostStatus(backend, [backend, mode, status]() {
              if (backend) {
                backend->OnModeStarted(mode, status);
              }
            });
          }
        });
    return;
  }
  if (mode == Mode::kDiscovery) {
    service_->RegisterSendSurface(
        &send_transfer_callback_, this,
        NearbySharingService::SendSurfaceState::kForeground,
        nearby::sharing::Advertisement::BlockedVendorId::kNone,
        /*disable_wifi_hotspot=*/false,
        [backend, mode](NearbySharingService::StatusCodes status) mutable {
          if (backend) {
            PostStatus(backend, [backend, mode, status]() {
              if (backend) {
                backend->OnModeStarted(mode, status);
              }
            });
          }
        });
    return;
  }

  mode_operation_in_flight_ = false;
}

void Backend::OnModeStopped(Mode mode,
                            NearbySharingService::StatusCodes status) {
  mode_operation_in_flight_ = false;
  if (shutting_down_) {
    return;
  }
  if (status == NearbySharingService::StatusCodes::kOk ||
      status == NearbySharingService::StatusCodes::kStatusAlreadyStopped) {
    active_mode_ = Mode::kNone;
    if (mode == Mode::kDiscovery) {
      targets_.Clear();
    }
  } else {
    desired_mode_ = active_mode_;
    ReportStatus(QStringLiteral("Stop sharing mode"), status);
  }
  DriveMode();
}

void Backend::OnModeStarted(Mode mode,
                            NearbySharingService::StatusCodes status) {
  mode_operation_in_flight_ = false;
  if (shutting_down_) {
    return;
  }
  if (status == NearbySharingService::StatusCodes::kOk) {
    active_mode_ = mode;
  } else {
    desired_mode_ = Mode::kNone;
    ReportStatus(QStringLiteral("Start sharing mode"), status);
  }
  DriveMode();
}

std::function<void(NearbySharingService::StatusCodes)> Backend::StatusCallback(
    QString operation) {
  QPointer<Backend> backend(this);
  return [backend, operation = std::move(operation)](
             NearbySharingService::StatusCodes status) {
    if (backend) {
      PostStatus(backend, [backend, operation, status]() {
        if (backend && !backend->shutting_down_) {
          backend->ReportStatus(operation, status);
        }
      });
    }
  };
}

void Backend::ReportStatus(const QString& operation,
                           NearbySharingService::StatusCodes status) {
  if (status != NearbySharingService::StatusCodes::kOk) {
    qWarning().noquote()
        << operation + QStringLiteral(" failed: ") +
               ToQString(NearbySharingService::StatusCodeToString(status));
  }
}
