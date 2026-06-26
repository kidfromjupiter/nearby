#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMetaObject>
#include <QObject>
#include <QTimer>
#include <QQmlApplicationEngine>

namespace {

constexpr char kQmlHotReloadEnv[] = "NEARBY_QML_HOT_RELOAD";
constexpr char kQmlSourceDirEnv[] = "NEARBY_QML_SOURCE_DIR";
constexpr char kDefaultQmlSourceDir[] =
    "/home/lasan/Dev/nearby_latest/sharing/linux/app";

bool IsHotReloadEnabled() {
  const QByteArray value = qgetenv(kQmlHotReloadEnv);
  return value == "1" || value.toLower() == "true";
}

QDir QmlSourceDir() {
  const QByteArray configured_dir = qgetenv(kQmlSourceDirEnv);
  if (!configured_dir.isEmpty()) {
    return QDir(QString::fromLocal8Bit(configured_dir));
  }
  return QDir(QString::fromUtf8(kDefaultQmlSourceDir));
}

void WatchQmlFiles(QFileSystemWatcher& watcher, const QDir& source_dir) {
  const QStringList existing_files = watcher.files();
  if (!existing_files.isEmpty()) {
    watcher.removePaths(existing_files);
  }

  const QStringList files =
      source_dir.entryList(QStringList() << "*.qml", QDir::Files, QDir::Name);
  for (const QString& file : files) {
    watcher.addPath(source_dir.absoluteFilePath(file));
  }
}

void DestroyRootObjects(QQmlApplicationEngine& engine) {
  const QList<QObject*> root_objects = engine.rootObjects();
  for (QObject* object : root_objects) {
    object->deleteLater();
  }
}

bool ReloadInnerContent(QQmlApplicationEngine& engine) {
  const QList<QObject*> root_objects = engine.rootObjects();
  if (root_objects.isEmpty()) {
    return false;
  }

  QObject* root = root_objects.constFirst();
  return QMetaObject::invokeMethod(root, "reloadInnerContent");
}

bool CanReloadInnerContent(const QFileInfo& changed_file) {
  return changed_file.fileName() == QStringLiteral("AppContent.qml");
}

}  // namespace

int main(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);

  QQmlApplicationEngine engine;
  const int fontId = QFontDatabase::addApplicationFont(":/googlesans_var.ttf");

  if (fontId != -1) {
    const QStringList fontFamilies =
        QFontDatabase::applicationFontFamilies(fontId);
    if (!fontFamilies.isEmpty()) {
      const QFont defaultFont(fontFamilies.at(0));
      app.setFont(defaultFont);
    }
  }

  if (IsHotReloadEnabled()) {
    const QDir qml_source_dir = QmlSourceDir();
    const QUrl source_url =
        QUrl::fromLocalFile(qml_source_dir.absoluteFilePath("main.qml"));
    QFileSystemWatcher watcher;
    QTimer reload_timer;
    reload_timer.setSingleShot(true);
    reload_timer.setInterval(75);
    QString changed_path;

    const auto fullReload = [&engine, &watcher, qml_source_dir, source_url]() {
      WatchQmlFiles(watcher, qml_source_dir);
      DestroyRootObjects(engine);
      engine.clearComponentCache();
      engine.load(source_url);
    };

    const auto reload = [&engine, &watcher, qml_source_dir, source_url,
                         &changed_path, &fullReload]() {
      WatchQmlFiles(watcher, qml_source_dir);
      const QFileInfo changed_file(changed_path);

      if (changed_file.fileName() == QStringLiteral("main.qml") ||
          !CanReloadInnerContent(changed_file) ||
          engine.rootObjects().isEmpty()) {
        fullReload();
        return;
      }

      engine.clearComponentCache();
      if (!ReloadInnerContent(engine)) {
        fullReload();
      }
    };

    QObject::connect(&reload_timer, &QTimer::timeout, &app, reload);
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, &app,
                     [&reload_timer, &changed_path](const QString& path) {
                       changed_path = path;
                       reload_timer.start();
                     });
    QObject::connect(&watcher, &QFileSystemWatcher::directoryChanged, &app,
                     [&reload_timer, &changed_path](const QString& path) {
                       changed_path = path;
                       reload_timer.start();
                     });

    watcher.addPath(qml_source_dir.absolutePath());
    fullReload();

    return app.exec();
  }

  // Loaded via the qrc scheme since it's compiled into the binary
  const QUrl url(QStringLiteral("qrc:/main.qml"));

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  engine.load(url);

  return app.exec();
}
