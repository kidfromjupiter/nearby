#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStyleHints>
#include <QSystemTrayIcon>

#include "file_share_tray_controller.h"

namespace {

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
  nearby::sharing::NearbyConnectionsQtFacade::SetBleL2capFlagOverrides(
      true, false);

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
                   &tray, [&tray](const QString& title, const QString& body) {
                     tray.showMessage(title, body, QSystemTrayIcon::Information,
                                      4000);
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

  controller.switchToReceiveMode();

  return app.exec();
}
