//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <gl/GLWindow.h>
#include <Platform.h>
#include <plugins/DisplayPlugin.h>
#include <gl/OglplusHelpers.h>
#include <OffscreenUi.h>
#include "shadertoy/Cache.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlNetworkAccessManagerFactory>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
Cache* _cache;


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


Application::Application(int& argc, char** argv) : PluginApplication( argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);

    //getWindow()->setGeometry(100, -980, 1280, 720);
    getWindow()->setGeometry(100, 100, 1280, 720);
    setOverrideCursor(Qt::BlankCursor);

    initializeUI(QUrl::fromLocalFile("shadertoy/AppDesktop.qml"));
    auto desktop = getOffscreenUi()->getDesktop();
    QObject::connect(desktop, SIGNAL(loadShader(QString)), this, SLOT(loadShader(const QString&)));
    _renderer.setup();
}

void Application::initializeUI(const QUrl& desktopUrl) {
    _cache = new Cache("C:/Users/Brad/git/shadertoys/");
    PluginApplication::initializeUI(desktopUrl);
    getActiveDisplayPlugin()->idle();
    getOffscreenUi()->setRootContextProperty("Renderer", &_renderer);
    getOffscreenUi()->setRootContextProperty("shadertoyCache", _cache);
    getOffscreenUi()->getRootContext()->engine()->setNetworkAccessManagerFactory(new MyQmlNetworkAccessManagerFactory());

}

void Application::loadShader(const QString& shaderId) {
    qDebug() << "Request load for shader " << shaderId;

}


void Application::cleanupBeforeQuit() {
    UiApplication::cleanupBeforeQuit();
}

void Application::aboutToQuit() {
    UiApplication::aboutToQuit();
    getWindow()->makeCurrent();
    Platform::runShutdownHooks();
    getWindow()->doneCurrent();
}

void Application::paintGL() {
    auto displayPlugin = getActiveDisplayPlugin();
    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();
    auto size = displayPlugin->getRecommendedRenderSize();
    using namespace oglplus;
    if (displayPlugin->isHmd()) {
        for_each_eye([&] (Eye eye){
            Context::Viewport(eye == Eye::Left ? 0 : size.x / 2, 0, size.x / 2, size.y);
            pr.top() = displayPlugin->getProjection(eye, mat4());
            mv.top() = glm::inverse(displayPlugin->getHeadPose(getFrameCount()));
            _renderer.render();
        });
    } else {
        Context::Viewport(size.x, size.y);
        _renderer.render();
    }
}
