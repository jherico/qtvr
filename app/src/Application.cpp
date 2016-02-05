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

#include <FileUtils.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>

#include "shadertoy/Shader.h"

//const QString SHADERS_DIR = "C:/Users/austi/git/shadertoys/";
//for (auto shaderFile : QDir(SHADERS_DIR).entryList(QStringList(QString("*.json")))) {
//    auto shaderDoc = QJsonDocument::fromJson(FileUtils::readFileToByteArray(SHADERS_DIR + shaderFile));
//
//    QFile formatted(SHADERS_DIR + shaderFile);
//    formatted.open(QFile::ReadWrite | QFile::Truncate);
//    QByteArray ba = shaderDoc.toJson(QJsonDocument::Indented);
//    auto wrote = formatted.write(ba);
//    Q_ASSERT(wrote == ba.size());
//    formatted.close();
//}

void initTextureCache();

Application::Application(int& argc, char** argv) : PluginApplication(argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);


    //getWindow()->setGeometry(100, -980, 1280, 720);
    getWindow()->setGeometry(100, 100, 1280, 720);

//    setOverrideCursor(Qt::BlankCursor);
    initializeUI(QUrl::fromLocalFile("shadertoy/AppDesktop.qml"));

    initTextureCache();

    _renderer.setup();
}

void Application::initializeUI(const QUrl& desktopUrl) {
    qmlRegisterType<Input>("Shadertoy", 1, 0, "Input");
    qmlRegisterType<Renderpass>("Shadertoy", 1, 0, "Renderpass");
    qmlRegisterType<ShaderInfo>("Shadertoy", 1, 0, "ShaderInfo");
    qmlRegisterType<Shader>("Shadertoy", 1, 0, "Shader");
    PluginApplication::initializeUI(desktopUrl);

    _model = new ShaderModel();
    getActiveDisplayPlugin()->idle();
    getOffscreenUi()->setRootContextProperty("renderer", &_renderer);
    getOffscreenUi()->setRootContextProperty("shaderModel", _model);
    getOffscreenUi()->setRootContextProperty("shadertoyCache", _model->_cache);
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
        _renderer.setResolution(size);
        _renderer.render();
    }
}

void Application::resizeGL() {
    PluginApplication::resizeGL();
    _renderer.setResolution(_renderResolution);
};



