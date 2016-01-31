#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngine>
#include <QFileSystemModel>
#include <QQmlNetworkAccessManagerFactory>

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

class MyNetworkAccessManager : public QNetworkAccessManager {
public:
    MyNetworkAccessManager(QObject *parent) : QNetworkAccessManager(parent) {}

    QNetworkReply* MyNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest& req, QIODevice* outgoingData) override {
        QNetworkRequest newRequest(req);
        newRequest.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.111 Safari/537.36");
        return QNetworkAccessManager::createRequest(op, newRequest, outgoingData);
    }
};

class MyQmlNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory {
    QNetworkAccessManager* create(QObject*parent) override {
        return new MyNetworkAccessManager(parent);
    }


};


int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Some Company");
    app.setOrganizationDomain("somecompany.com");
    app.setApplicationName("Amazing Application");
    QDir::setCurrent(getRelativeDir(".."));
    QGuiApplication::setOverrideCursor(Qt::BlankCursor);
    QtWebEngine::initialize();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("DebugQML", true);
    engine.setNetworkAccessManagerFactory(new MyQmlNetworkAccessManagerFactory());
    addImportPath(engine, "../qml");
    addImportPath(engine, "../../../app/resources/qml");
    engine.load(QUrl(QStringLiteral("qml/main.qml")));
    engine.load(QUrl(QStringLiteral("qml/Stubs.qml")));
    setChild(engine, "Renderer");
    setChild(engine, "offscreenFlags");
    return app.exec();
}
