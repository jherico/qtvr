#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngine>
#include <QFileSystemModel>
#include <QQmlNetworkAccessManagerFactory>

#include "../../app/src/shadertoy/Cache.h"

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

QString getResourceDirectory() {
    return getRelativeDir("../../app/resources/");
}

QObject* getChildByName(QQmlApplicationEngine& engine, const char* name) {
    for (auto obj : engine.rootObjects()) {
        auto child = obj->findChild<QObject*>(QString(name));
        if (child) {
            return child;
        }
    }
    qWarning() << "Could not find object named " << name;
    return nullptr;
}


void setChild(QQmlApplicationEngine& engine, const char* name) {
    auto child = getChildByName(engine, name);
    if (child) {
        engine.rootContext()->setContextProperty(name, name);
    }
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

class Persister : public QObject {
    Q_OBJECT
public:
    Persister(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void persistShaderList(QString shaderList) {
        qDebug() << "Persist shader list";
        QFile file("C:/Users/bdavis/Git/core/app/resources/shadertoys/shadertoys.json");
        if (!file.open(QFile::Truncate | QFile::ReadWrite)) {
            qWarning() << "Could not open output file";
            return;
        }
        file.write(shaderList.toUtf8());
        file.close();
    }

    void persistShader(QString shader) {
        qDebug() << "Persist shader ";
        auto shaderBytes = shader.toUtf8();
        QJsonDocument document = QJsonDocument::fromJson(shaderBytes);
        auto object = document.object();
        object = object.value("Shader").toObject();
        object = object.value("info").toObject();
        auto id = object.value("id").toString();
        if (id.isEmpty()) {
            qWarning() << "Could not find shader ID";
            return;
        }
        QFile file("C:/Users/bdavis/Git/core/app/resources/shadertoys/" + id + ".json");
        if (!file.open(QFile::Truncate | QFile::ReadWrite)) {
            qWarning() << "Could not open output file";
            return;
        }
        file.write(shaderBytes);
        file.close();
    }

};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Some Company");
    app.setOrganizationDomain("somecompany.com");
    app.setApplicationName("Amazing Application");
    QDir::setCurrent(getRelativeDir(".."));
    //QGuiApplication::setOverrideCursor(Qt::BlankCursor);
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

    auto persister = new Persister();
    auto shadertoy = getChildByName(engine, "shadertoy");
    QObject::connect(shadertoy, SIGNAL(receivedShaderList(QString)), persister, SLOT(persistShaderList(QString)));
    QObject::connect(shadertoy, SIGNAL(receivedShader(QString)), persister, SLOT(persistShader(QString)));
    return app.exec();
}

#include "main.moc"
