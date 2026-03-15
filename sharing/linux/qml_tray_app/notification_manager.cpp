#include "notification_manager.h"

#include <QAbstractButton>
#include <QClipboard>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QVariantMap>

namespace {

constexpr char kNotificationsService[] = "org.freedesktop.Notifications";
constexpr char kNotificationsPath[] = "/org/freedesktop/Notifications";
constexpr char kNotificationsInterface[] = "org.freedesktop.Notifications";
constexpr char kCopyActionId[] = "copy_value";
constexpr char kDesktopEntryId[] = "nearby-file-share";

}  // namespace

NotificationManager::NotificationManager(QSystemTrayIcon* tray_icon,
                                         QObject* parent)
    : QObject(parent), tray_icon_(tray_icon) {
  QDBusConnection session_bus = QDBusConnection::sessionBus();
  if (!session_bus.isConnected()) {
    return;
  }

  session_bus.connect(QString::fromLatin1(kNotificationsService),
                      QString::fromLatin1(kNotificationsPath),
                      QString::fromLatin1(kNotificationsInterface),
                      QStringLiteral("ActionInvoked"), this,
                      SLOT(OnActionInvoked(uint,QString)));
  session_bus.connect(QString::fromLatin1(kNotificationsService),
                      QString::fromLatin1(kNotificationsPath),
                      QString::fromLatin1(kNotificationsInterface),
                      QStringLiteral("NotificationClosed"), this,
                      SLOT(OnNotificationClosed(uint,uint)));

  QDBusInterface notification_interface(
      QString::fromLatin1(kNotificationsService),
      QString::fromLatin1(kNotificationsPath),
      QString::fromLatin1(kNotificationsInterface), session_bus);
  QDBusReply<QStringList> capabilities_reply =
      notification_interface.call(QStringLiteral("GetCapabilities"));
  if (capabilities_reply.isValid()) {
    supports_actions_ =
        capabilities_reply.value().contains(QStringLiteral("actions"));
  }
}

void NotificationManager::ShowNotification(const QString& title,
                                           const QString& body) {
  if (tray_icon_ != nullptr) {
    tray_icon_->showMessage(title, body, QSystemTrayIcon::Information, 4000);
  }
}

void NotificationManager::ShowCopyableNotification(
    const QString& title, const QString& body, const QString& text_to_copy,
    const QString& action_label) {
  const QString trimmed_text = text_to_copy.trimmed();
  const QString trimmed_action_label = action_label.trimmed().isEmpty()
                                           ? QStringLiteral("Copy")
                                           : action_label.trimmed();
  if (trimmed_text.isEmpty()) {
    ShowNotification(title, body);
    return;
  }

  if (supports_actions_) {
    QDBusInterface notification_interface(
        QString::fromLatin1(kNotificationsService),
        QString::fromLatin1(kNotificationsPath),
        QString::fromLatin1(kNotificationsInterface),
        QDBusConnection::sessionBus());
    const QString application_name = QCoreApplication::applicationName();
    const QString notification_icon = EnsureNotificationIconPath();
    QVariantMap hints{{QStringLiteral("desktop-entry"),
                       QString::fromLatin1(kDesktopEntryId)}};
    if (!notification_icon.isEmpty()) {
      hints.insert(QStringLiteral("image-path"), notification_icon);
    }
    QDBusReply<uint> reply = notification_interface.call(
        QStringLiteral("Notify"), application_name,
        static_cast<uint>(0),
        notification_icon.isEmpty() ? QString::fromLatin1(kDesktopEntryId)
                                    : notification_icon,
        title, body,
        QStringList{QString::fromLatin1(kCopyActionId), trimmed_action_label},
        hints, 8000);
    if (reply.isValid()) {
      copy_actions_.insert(reply.value(), CopyActionState{trimmed_text});
      return;
    }
  }

  ShowFallbackDialog(title, body, trimmed_text, trimmed_text,
                     trimmed_action_label);
}

void NotificationManager::OnActionInvoked(uint notification_id,
                                          const QString& action_key) {
  if (action_key != QString::fromLatin1(kCopyActionId)) {
    return;
  }

  auto it = copy_actions_.find(notification_id);
  if (it == copy_actions_.end()) {
    return;
  }

  CopyTextToClipboard(it->text_to_copy, QStringLiteral("Copied"),
                      QStringLiteral("Copied to clipboard."));
  copy_actions_.erase(it);
}

void NotificationManager::OnNotificationClosed(uint notification_id,
                                               uint reason) {
  Q_UNUSED(reason);
  copy_actions_.remove(notification_id);
}

void NotificationManager::CopyTextToClipboard(
    const QString& text_to_copy, const QString& confirmation_title,
    const QString& confirmation_body) const {
  QClipboard* clipboard = QGuiApplication::clipboard();
  if (clipboard != nullptr) {
    clipboard->setText(text_to_copy);
  }

  if (tray_icon_ != nullptr) {
    tray_icon_->showMessage(confirmation_title, confirmation_body,
                            QSystemTrayIcon::Information, 2500);
  }
}

QString NotificationManager::EnsureNotificationIconPath() {
  if (!notification_icon_path_.isEmpty() &&
      QFileInfo::exists(notification_icon_path_)) {
    return notification_icon_path_;
  }

  if (tray_icon_ == nullptr || tray_icon_->icon().isNull()) {
    return {};
  }

  QString cache_dir =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (cache_dir.isEmpty()) {
    cache_dir = QDir::tempPath() + QStringLiteral("/nearby-file-share");
  }

  QDir dir(cache_dir);
  if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
    return {};
  }

  const QString icon_path = dir.filePath(QStringLiteral("notification-icon.png"));
  const QPixmap icon_pixmap = tray_icon_->icon().pixmap(128, 128);
  if (icon_pixmap.isNull() || !icon_pixmap.save(icon_path, "PNG")) {
    return {};
  }

  notification_icon_path_ = icon_path;
  return notification_icon_path_;
}

void NotificationManager::ShowFallbackDialog(const QString& title,
                                             const QString& body,
                                             const QString& informative_text,
                                             const QString& text_to_copy,
                                             const QString& action_label) {
  auto* message_box =
      new QMessageBox(QMessageBox::Information, title, body, QMessageBox::NoButton);
  message_box->setAttribute(Qt::WA_DeleteOnClose);
  message_box->setTextFormat(Qt::PlainText);
  message_box->setInformativeText(informative_text);
  message_box->setWindowFlag(Qt::WindowStaysOnTopHint);
  if (tray_icon_ != nullptr && !tray_icon_->icon().isNull()) {
    message_box->setWindowIcon(tray_icon_->icon());
  }

  QAbstractButton* copy_button =
      message_box->addButton(action_label, QMessageBox::ActionRole);
  message_box->addButton(QMessageBox::Close);

  QObject::connect(message_box, &QMessageBox::buttonClicked, message_box,
                   [this, message_box, copy_button, text_to_copy](
                       QAbstractButton* button) {
                     if (button == copy_button) {
                       CopyTextToClipboard(text_to_copy, QStringLiteral("Copied"),
                                           QStringLiteral("Copied to clipboard."));
                     }
                     message_box->close();
                   });

  message_box->show();
  message_box->raise();
  message_box->activateWindow();
}
