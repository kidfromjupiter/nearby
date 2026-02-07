#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSystemTrayIcon>

#include "sharing/linux/qml_tray_app/nearby_tray_controller.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  NearbyTrayController controller;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("nearbyController", &controller);
  engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }

  auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
  if (window == nullptr) {
    return 1;
  }

  QIcon tray_icon = QIcon::fromTheme(QStringLiteral("network-wireless"));
  if (tray_icon.isNull()) {
    tray_icon = app.windowIcon();
  }
  QSystemTrayIcon tray(tray_icon);
  tray.setToolTip(QStringLiteral("Nearby QML Tray"));

  QMenu tray_menu;
  QAction* show_action = tray_menu.addAction(QStringLiteral("Show"));
  QAction* hide_action = tray_menu.addAction(QStringLiteral("Hide"));
  tray_menu.addSeparator();
  QAction* quit_action = tray_menu.addAction(QStringLiteral("Quit"));

  QObject::connect(show_action, &QAction::triggered, window, [window]() {
    window->show();
    window->raise();
    window->requestActivate();
  });
  QObject::connect(hide_action, &QAction::triggered, window, [window]() {
    window->hide();
  });
  QObject::connect(quit_action, &QAction::triggered, &app, [&controller, &app]() {
    controller.stop();
    app.quit();
  });

  QObject::connect(&tray, &QSystemTrayIcon::activated, window,
                   [&tray, window](QSystemTrayIcon::ActivationReason reason) {
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

  QObject::connect(&controller, &NearbyTrayController::requestTrayMessage, &tray,
                   [&tray](const QString& title, const QString& body) {
                     tray.showMessage(title, body, QSystemTrayIcon::Information,
                                      3000);
                   });

  QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller,
                   [&controller]() { controller.stop(); });

  tray.setContextMenu(&tray_menu);
  tray.show();

  return app.exec();
}
