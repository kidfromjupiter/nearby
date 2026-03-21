#ifndef SHARING_LINUX_QML_TRAY_APP_SHARE_TARGET_MODEL_H_
#define SHARING_LINUX_QML_TRAY_APP_SHARE_TARGET_MODEL_H_

#include <QString>
#include <QVariantMap>

class ShareTarget {
 public:
  explicit ShareTarget(qlonglong id, const QString& name, bool is_incoming)
      : id_(id), name_(name), is_incoming_(is_incoming) {}

  qlonglong id() const { return id_; }
  QString name() const { return name_; }
  bool isIncoming() const { return is_incoming_; }

  QVariantMap toVariantMap() const {
    QVariantMap map;
    map[QStringLiteral("id")] = id_;
    map[QStringLiteral("name")] = name_;
    map[QStringLiteral("isIncoming")] = is_incoming_;
    return map;
  }

 private:
  qlonglong id_;
  QString name_;
  bool is_incoming_;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_SHARE_TARGET_MODEL_H_
