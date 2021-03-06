//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlNetworkAccessManagerFactory>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>

#include <gl/GLWindow.h>
#include <Platform.h>
#include <plugins/DisplayPlugin.h>
#include <gl/OglplusHelpers.h>
#include <OffscreenUi.h>
#include <FileUtils.h>

#include "shadertoy/Cache.h"
#include "shadertoy/types/Shader.h"

Application::Application(int& argc, char** argv) : Parent(argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);
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

    _proxy = std::make_shared<QSortFilterProxyModel>(this);
    _model = std::make_shared<shadertoy::Model>();
    _proxy->setSourceModel(_model.get());

    //getWindow()->setGeometry(0, 0, 1280, 720);
    getWindow()->setGeometry(2020, 100, 1280, 720);
    //getWindow()->setGeometry(100, 100, 1280, 720);
    initializeUI(QUrl::fromLocalFile("shadertoy/AppDesktop.qml"));

    makePrimaryRenderingContextCurrent();
    _renderer.setup(uvec2(1280, 720));
}

void Application::initializeUI(const QUrl& desktopUrl) {
    qmlRegisterType<Input>("Shadertoy", 1, 0, "Input");
    qmlRegisterType<Renderpass>("Shadertoy", 1, 0, "Renderpass");
    qmlRegisterType<ShaderInfo>("Shadertoy", 1, 0, "ShaderInfo");
    qmlRegisterType<Shader>("Shadertoy", 1, 0, "Shader");
    Parent::initializeUI(desktopUrl);

    getActiveDisplayPlugin()->idle();
    auto offscreenUi = getOffscreenUi();
    auto rootContext = offscreenUi->getRootContext();
    rootContext->setContextProperty("renderer", &_renderer);
    rootContext->setContextProperty("shaderModel", _model.get());
    rootContext->setContextProperty("shadertoyCache", _model->_cache);
    auto desktop = offscreenUi->getDesktop();
    connect(desktop, SIGNAL(updateCode(QString)), this, SLOT(onUpdateCode(const QString&)));
}

void Application::onUpdateCode(const QString& code) {
    // FIXME support buffer editing
    _renderer.setShaderCode(Renderpass::IMAGE, code);
}

void Application::loadShader(const QString& shaderId) {
    qDebug() << "Request load for shader " << shaderId;
}

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



