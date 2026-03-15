#ifndef SHARING_LINUX_QML_TRAY_APP_NOTIFICATION_MANAGER_H_
#define SHARING_LINUX_QML_TRAY_APP_NOTIFICATION_MANAGER_H_

#include <QObject>

#include <QHash>
#include <QString>

class QSystemTrayIcon;

class NotificationManager : public QObject {
  Q_OBJECT

 public:
  explicit NotificationManager(QSystemTrayIcon* tray_icon,
                               QObject* parent = nullptr);

  void ShowNotification(const QString& title, const QString& body);
  void ShowCopyableNotification(const QString& title, const QString& body,
                                const QString& text_to_copy,
                                const QString& action_label);

 private slots:
  void OnActionInvoked(uint notification_id, const QString& action_key);
  void OnNotificationClosed(uint notification_id, uint reason);

 private:
  struct CopyActionState {
    QString text_to_copy;
  };

  void CopyTextToClipboard(const QString& text_to_copy,
                           const QString& confirmation_title,
                           const QString& confirmation_body) const;
  QString EnsureNotificationIconPath();
  void ShowFallbackDialog(const QString& title, const QString& body,
                          const QString& informative_text,
                          const QString& text_to_copy,
                          const QString& action_label);

  bool supports_actions_ = false;
  QHash<uint, CopyActionState> copy_actions_;
  QString notification_icon_path_;
  QSystemTrayIcon* tray_icon_ = nullptr;
};

#endif  // SHARING_LINUX_QML_TRAY_APP_NOTIFICATION_MANAGER_H_
