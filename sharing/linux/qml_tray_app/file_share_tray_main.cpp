#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QStyleHints>
#include <QSystemTrayIcon>

#include <fcntl.h>
#include <unistd.h>

#include "file_share_tray_controller.h"
#include "notification_manager.h"

namespace {

constexpr char kDefaultLogPath[] = "/tmp/nearby_qml_file_tray.log";

bool EnsureLogDirectory(const QString& file_path) {
  const QFileInfo file_info(file_path);
  QDir directory = file_info.absoluteDir();
  if (directory.exists()) {
    return true;
  }
  return directory.mkpath(QStringLiteral("."));
}

bool RedirectStdStreamsToFile(const QString& file_path) {
  const QByteArray encoded_path = QFile::encodeName(file_path);
  const int fd = ::open(encoded_path.constData(), O_CREAT | O_APPEND | O_WRONLY, 0644);
  if (fd < 0) {
    return false;
  }

  const bool redirected_stdout = ::dup2(fd, STDOUT_FILENO) >= 0;
  const bool redirected_stderr = ::dup2(fd, STDERR_FILENO) >= 0;
  ::close(fd);
  return redirected_stdout && redirected_stderr;
}

QString ResolveConfiguredLogPath() {
  QSettings settings(QStringLiteral("Nearby"), QStringLiteral("QmlFileTrayApp"));
  const QString configured_path =
      settings.value(QStringLiteral("logPath"),
                     QString::fromLatin1(kDefaultLogPath))
          .toString()
          .trimmed();
  if (configured_path.isEmpty()) {
    return QString::fromLatin1(kDefaultLogPath);
  }
  return configured_path;
}

void RedirectProcessLogsToConfiguredPath() {
  QString log_path = ResolveConfiguredLogPath();
  if (EnsureLogDirectory(log_path) && RedirectStdStreamsToFile(log_path)) {
    return;
  }

  const QString fallback_path = QString::fromLatin1(kDefaultLogPath);
  if (log_path == fallback_path) {
    return;
  }
  if (!EnsureLogDirectory(fallback_path)) {
    return;
  }
  RedirectStdStreamsToFile(fallback_path);
}

QIcon BuildTintedSymbolicIcon(const QString& source, const QColor& color) {
  QIcon source_icon(source);
  if (source_icon.isNull()) {
    return QIcon();
  }

  QIcon tinted_icon;
  for (int size : {16, 18, 20, 22, 24, 32}) {
    QPixmap pixmap = source_icon.pixmap(size, size);
    if (pixmap.isNull()) {
      continue;
    }

    QPixmap tinted(pixmap.size());
    tinted.fill(Qt::transparent);

    QPainter painter(&tinted);
    painter.drawPixmap(0, 0, pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    painter.end();

    tinted_icon.addPixmap(tinted);
  }

  return tinted_icon;
}

}  // namespace

int main(int argc, char* argv[]) {
  RedirectProcessLogsToConfiguredPath();

  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  FileShareTrayController controller;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("fileShareController", &controller);
  engine.load(QUrl(QStringLiteral("qrc:/qml/FileShareTray.qml")));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }


  auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
  if (window == nullptr) {
    return 1;
  }

  const auto resolve_tray_icon = [&app]() {
    const QColor symbolic_color = app.palette().color(QPalette::WindowText);
    QIcon tray_icon = BuildTintedSymbolicIcon(
        QStringLiteral(":/icons/tray_icon-symbolic.svg"), symbolic_color);
    if (tray_icon.isNull()) {
      tray_icon = QIcon::fromTheme(QStringLiteral("network-wireless-symbolic"));
    }
    if (tray_icon.isNull()) {
      tray_icon = QIcon(QStringLiteral(":/icons/tray_icon.png"));
    }
    if (tray_icon.isNull()) {
      tray_icon = app.windowIcon();
    }
    return tray_icon;
  };

  QSystemTrayIcon tray(resolve_tray_icon());
  tray.setToolTip(QStringLiteral("Nearby File Tray"));
  NotificationManager notification_manager(&tray, &app);

  QMenu tray_menu;
  QAction* send_action = tray_menu.addAction(QStringLiteral("Send"));
  QAction* receive_action = tray_menu.addAction(QStringLiteral("Receive"));
  tray_menu.addSeparator();
  QAction* show_action = tray_menu.addAction(QStringLiteral("Show"));
  QAction* hide_action = tray_menu.addAction(QStringLiteral("Hide"));
  tray_menu.addSeparator();
  QAction* quit_action = tray_menu.addAction(QStringLiteral("Quit"));

  QObject::connect(send_action, &QAction::triggered, window,
                   [&controller, window]() {
                     const QString file = QFileDialog::getOpenFileName(
                         nullptr, QStringLiteral("Select file to send"));
                     if (file.isEmpty()) {
                       return;
                     }
                     controller.switchToSendModeWithFile(file);
                     window->show();
                     window->raise();
                     window->requestActivate();
                   });

  QObject::connect(receive_action, &QAction::triggered,
                   [&controller, window]() {
                     controller.switchToReceiveMode();
                     window->show();
                     window->raise();
                     window->requestActivate();
                   });

  QObject::connect(show_action, &QAction::triggered, window, [window]() {
    window->show();
    window->raise();
    window->requestActivate();
  });

  QObject::connect(hide_action, &QAction::triggered, window, [window]() {
    window->hide();
  });

  QObject::connect(quit_action, &QAction::triggered, &app,
                   [&controller, &app]() {
                     controller.stop();
                     app.quit();
                   });

  QObject::connect(&tray, &QSystemTrayIcon::activated, window,
                   [window](QSystemTrayIcon::ActivationReason reason) {
                     if (reason != QSystemTrayIcon::Trigger &&
                         reason != QSystemTrayIcon::DoubleClick) {
                       return;
                     }
                     if (window->isVisible()) {
                       window->hide();
                     } else {
                       window->show();
                       window->raise();
                       window->requestActivate();
                     }
                   });

  QObject::connect(&controller, &FileShareTrayController::requestTrayMessage,
                   &notification_manager, &NotificationManager::ShowNotification);
  QObject::connect(&controller,
                   &FileShareTrayController::requestCopyLinkTrayMessage,
                   &notification_manager,
                   [&notification_manager](const QString& title,
                                           const QString& body,
                                           const QString& link) {
                     notification_manager.ShowCopyableNotification(
                         title, body, link, QStringLiteral("Copy link"));
                   });

  QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller,
                   [&controller]() { controller.stop(); });
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, &tray,
                   [&tray, &resolve_tray_icon](Qt::ColorScheme) {
                     tray.setIcon(resolve_tray_icon());
                   });
#endif

  tray.setContextMenu(&tray_menu);
  tray.show();

  controller.start();
  controller.switchToReceiveMode();

  return app.exec();
}
