#ifndef SHARING_LINUX_QML_TRAY_APP_TRANSFER_MODEL_H_
#define SHARING_LINUX_QML_TRAY_APP_TRANSFER_MODEL_H_

#include <QString>
#include <QVariantMap>

class Transfer {
 public:
  Transfer(qlonglong target_id, const QString& target_name,
           const QString& status, double progress, qulonglong transferred_bytes,
           const QString& direction, const QString& file_name)
      : target_id_(target_id),
        target_name_(target_name),
        status_(status),
        progress_(progress),
        transferred_bytes_(transferred_bytes),
        direction_(direction),
        file_name_(file_name) {}

  qlonglong targetId() const { return target_id_; }
  QString targetName() const { return target_name_; }
  QString status() const { return status_; }
  double progress() const { return progress_; }
  qulonglong transferredBytes() const { return transferred_bytes_; }
  QString direction() const { return direction_; }
  QString fileName() const { return file_name_; }

  QVariantMap toVariantMap() const {
    QVariantMap map;
    map[QStringLiteral("targetId")] = target_id_;
    map[QStringLiteral("targetName")] = target_name_;
    map[QStringLiteral("status")] = status_;
    map[QStringLiteral("progress")] = progress_;
    map[QStringLiteral("transferredBytes")] = static_cast<qulonglong>(transferred_bytes_);
    map[QStringLiteral("direction")] = direction_;
    map[QStringLiteral("fileName")] = file_name_;
    return map;
  }

 private:
  qlonglong target_id_;
  QString target_name_;
  QString status_;
  double progress_;
  qulonglong transferred_bytes_;
  QString direction_;
  QString file_name_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_TRANSFER_MODEL_H_
