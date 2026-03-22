#include "file_share_state.h"
#include "status_mapper.h"

FileShareState::FileShareState() = default;

void FileShareState::AddOrUpdateTarget(qlonglong id, const QString& name,
                                       bool is_incoming) {
  target_names_[id] = name;

  if (discovered_row_by_target_.contains(id)) {
    const int row_index = discovered_row_by_target_.value(id);
    if (row_index >= 0 && row_index < discovered_targets_.size()) {
      QVariantMap target;
      target[QStringLiteral("id")] = id;
      target[QStringLiteral("name")] = name;
      target[QStringLiteral("isIncoming")] = is_incoming;
      discovered_targets_[row_index] = target;
      return;
    }
  }

  QVariantMap target;
  target[QStringLiteral("id")] = id;
  target[QStringLiteral("name")] = name;
  target[QStringLiteral("isIncoming")] = is_incoming;
  discovered_row_by_target_[id] = discovered_targets_.size();
  discovered_targets_.append(target);
}

void FileShareState::RemoveTarget(qlonglong id) {
  if (HasActiveTransferForTarget(id)) {
    AddPendingTargetRemoval(id);
    return;
  }

  RemovePendingTargetRemoval(id);
  target_names_.remove(id);

  if (!discovered_row_by_target_.contains(id)) {
    return;
  }

  const int removed_index = discovered_row_by_target_.take(id);
  if (removed_index < 0 || removed_index >= discovered_targets_.size()) {
    return;
  }

  discovered_targets_.removeAt(removed_index);
  for (auto it = discovered_row_by_target_.begin();
       it != discovered_row_by_target_.end(); ++it) {
    if (it.value() > removed_index) {
      it.value() = it.value() - 1;
    }
  }
}

QString FileShareState::GetTargetName(qlonglong id) const {
  const QString name = target_names_.value(id).trimmed();
  return name.isEmpty() ? QStringLiteral("Unknown device") : name;
}

bool FileShareState::HasTarget(qlonglong id) const {
  return discovered_row_by_target_.contains(id);
}

void FileShareState::AddOrUpdateTransfer(
    qlonglong target_id, const QString& target_name, const QString& status,
    double progress, qulonglong transferred_bytes, const QString& direction,
    const QString& file_name, const QString& file_path) {
  QVariantMap transfer{
      {QStringLiteral("targetId"), target_id},
      {QStringLiteral("targetName"), target_name},
      {QStringLiteral("status"), status},
      {QStringLiteral("progress"), progress},
      {QStringLiteral("transferredBytes"), transferred_bytes},
      {QStringLiteral("direction"), direction},
      {QStringLiteral("fileName"), file_name},
      {QStringLiteral("filePath"), file_path},
  };

  qInfo() << "qDiscovered targets" << discoveredTargets();
  if (transfer_row_by_target_.contains(target_id)) {
    const int row_index = transfer_row_by_target_.value(target_id);
    if (row_index >= 0 && row_index < transfers_.size()) {
      transfers_[row_index] = transfer;
      return;
    }
  }

  transfer_row_by_target_.insert(target_id, transfers_.size());
  transfers_.append(transfer);
}

void FileShareState::RemoveTransfer(qlonglong target_id) {
  if (!transfer_row_by_target_.contains(target_id)) {
    return;
  }

  const int removed_index = transfer_row_by_target_.take(target_id);
  if (removed_index < 0 || removed_index >= transfers_.size()) {
    return;
  }

  transfers_.removeAt(removed_index);
  for (auto it = transfer_row_by_target_.begin();
       it != transfer_row_by_target_.end(); ++it) {
    if (it.value() > removed_index) {
      it.value() = it.value() - 1;
    }
  }
}

bool FileShareState::HasActiveTransferForTarget(qlonglong target_id) const {
  for (const QVariant& row_value : transfers_) {
    const QVariantMap row = row_value.toMap();
    if (row.value(QStringLiteral("targetId")).toLongLong() != target_id) {
      continue;
    }

    const QString status = row.value(QStringLiteral("status")).toString();
    if (StatusMapper::IsActiveTransferStatus(status)) {
      return true;
    }
  }
  return false;
}

bool FileShareState::HasActiveTransfers() const {
  for (const QVariant& row_value : transfers_) {
    const QVariantMap row = row_value.toMap();
    const QString status = row.value(QStringLiteral("status")).toString();
    if (StatusMapper::IsActiveTransferStatus(status)) {
      return true;
    }
  }
  return false;
}

void FileShareState::AddPendingTargetRemoval(qlonglong id) {
  pending_target_removals_.insert(id);
}

void FileShareState::RemovePendingTargetRemoval(qlonglong id) {
  pending_target_removals_.remove(id);
}

bool FileShareState::IsPendingTargetRemoval(qlonglong id) const {
  return pending_target_removals_.contains(id);
}

void FileShareState::ClearAll() {
  discovered_targets_.clear();
  discovered_row_by_target_.clear();
  target_names_.clear();
  transfers_.clear();
  transfer_row_by_target_.clear();
  pending_target_removals_.clear();
}
