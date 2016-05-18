//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <QtQml/QQmlContext>

#include <gl/GLWindow.h>
#include <Platform.h>
#include <plugins/DisplayPlugin.h>
#include <OffscreenUi.h>

#include "shadertoy/Cache.h"
#include "shadertoy/types/Shader.h"

Application::Application(int& argc, char** argv) : Parent(argc, argv) {
    // Q_INIT_RESOURCE(ShadertoyVR);
    //const QString SHADERS_DIR = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/git/shadertoys/";

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

    QCoreApplication::setApplicationName("ShadertoyVR");
    QCoreApplication::setOrganizationName("Saint Andreas");
    QCoreApplication::setOrganizationDomain("saintandreas.org");

//    _proxy = std::make_shared<QSortFilterProxyModel>(this);
//    _model = std::make_shared<shadertoy::Model>();
//    _proxy->setSourceModel(_model.get());

    getWindow()->setGeometry(QRect(getWindow()->position(), QSize(1440, 800)));
    //getWindow()->setGeometry(0, 0, 1280, 720);
    //getWindow()->setGeometry(2020, 100, 640, 480);
//    getWindow()->setGeometry(100, -980, 1440, 800);
    //getWindow()->setGeometry(100, 100, 1280, 720);

    initializeUI(QUrl::fromLocalFile("C:/Users/bdavis/Git/core/app/resources/qml/shadertoy/AppDesktop.qml"));

    setActiveDisplayPlugin("Oculus Rift");
    makePrimaryRenderingContextCurrent();
    _renderer.setup(uvec2(1280, 720));
}

void Application::initializeUI(const QUrl& desktopUrl) {
    qmlRegisterType<Input>("Shadertoy", 1, 0, "Input");
    qmlRegisterType<Renderpass>("Shadertoy", 1, 0, "Renderpass");
    qmlRegisterType<ShaderInfo>("Shadertoy", 1, 0, "ShaderInfo");
    qmlRegisterType<Shader>("Shadertoy", 1, 0, "Shader");
    qDebug() << desktopUrl;
    Parent::initializeUI(desktopUrl);

    getActiveDisplayPlugin()->idle();
    auto offscreenUi = getOffscreenUi();
    auto rootContext = offscreenUi->getRootContext();
    rootContext->setContextProperty("renderer", &_renderer);
    //rootContext->setContextProperty("shaderModel", _model.get());
    //rootContext->setContextProperty("shadertoyCache", _model->_cache);
    //auto desktop = offscreenUi->getDesktop();
    //desktop->setProperty("shaderModel", QVariant::fromValue(_model.get()));
    //connect(desktop, SIGNAL(selectedShader(QVariant)), this, SLOT(loadShaderObject(const QVariant&)));
    //connect(desktop, SIGNAL(updateCode(QString)), this, SLOT(onUpdateCode(const QString&)));
}

//void Application::onUpdateCode(const QString& code) {
//    // FIXME support buffer editing
//    _renderer.setShaderCode(Renderpass::IMAGE, code);
//}
//
//void Application::loadShaderObject(const QVariant& shader) {
//    qDebug() << "Request load for shader " << shader;
//}
//
//void Application::loadShader(const QString& shaderId) {
//    qDebug() << "Request load for shader " << shaderId;
//}

void Application::aboutToQuit() {
    Parent::aboutToQuit();
    getWindow()->makeCurrent();
    Platform::runShutdownHooks();
    getWindow()->doneCurrent();
}

void Application::paintGL() {
    auto displayPlugin = getActiveDisplayPlugin();
    Stacks::modelview().top() = glm::inverse(displayPlugin->getHeadPose());
    _renderer.render();
}

void Application::resizeGL() {
    Parent::resizeGL();
    auto displayPlugin = getActiveDisplayPlugin();
    auto size = displayPlugin->getRecommendedRenderSize();
    _renderer.setSize(QSize(size.x, size.y));
};



