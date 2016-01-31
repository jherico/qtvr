#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngine>
#include <QFileSystemModel>

QString getRelativeDir(const QString& relativePath = ".") {
    QDir path(__FILE__); path.cdUp();
    auto result = path.absoluteFilePath(relativePath);
    result = path.cleanPath(result) + "/";
    return result;
}

QString getTestQmlDir() {
    return getRelativeDir("../qml");
}

QString getInterfaceQmlDir() {
    return getRelativeDir("/");
}


void setChild(QQmlApplicationEngine& engine, const char* name) {
  for (auto obj : engine.rootObjects()) {
    auto child = obj->findChild<QObject*>(QString(name));
    if (child) {
      engine.rootContext()->setContextProperty(name, child);
      return;
    }
  }
  qWarning() << "Could not find object named " << name;
}

void addImportPath(QQmlApplicationEngine& engine, const QString& relativePath) {
    QString resolvedPath = getRelativeDir(relativePath);
    QUrl resolvedUrl = QUrl::fromLocalFile(resolvedPath);
    resolvedPath = resolvedUrl.toString();
    engine.addImportPath(resolvedPath);
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Some Company");
    app.setOrganizationDomain("somecompany.com");
    app.setApplicationName("Amazing Application");
    QDir::setCurrent(getRelativeDir(".."));

    QtWebEngine::initialize();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("DebugQML", true);
    addImportPath(engine, "../qml");
    addImportPath(engine, "../../../app/resources/qml");
    engine.load(QUrl(QStringLiteral("qml/main.qml")));
    engine.load(QUrl(QStringLiteral("qml/Stubs.qml")));
    setChild(engine, "Renderer");
    setChild(engine, "offscreenFlags");
    return app.exec();
}
