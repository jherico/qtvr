//
//  Application.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PluginApplication.h"

#include <gl/Config.h>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QMimeData>
#include <QtCore/QLoggingCategory>

#include <QtGui/QScreen>
#include <QtGui/QImage>
#include <QtGui/QWheelEvent>
#include <QtGui/QWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QDesktopServices>

#include <QtGui/QOpenGLFramebufferObject>
#include <shared/NsightHelpers.h>

#include <gl/Config.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLWindow.h>
#include <gl/OffscreenGLCanvas.h>

#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginManager.h"

#include <controllers/UserInputMapper.h>
#include <controllers/StateController.h>

#include <Menu.h>

#include <SettingManager.h>
#include <SettingHandle.h>
#include <Finally.h>
#include <LogHandler.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <ResourceCache.h>
#include <UUID.h>

PluginApplication::PluginApplication(int& argc, char** argv)
    : GLApplication(argc, argv) {

    DependencyManager::set<UserInputMapper>();

    initializeGL();
    _offscreenContext->makeCurrent();

    // A new controllerInput device used to reflect current values from the application state
    _applicationStateDevice = std::make_shared<controller::StateController>();

    _applicationStateDevice->addInputVariant(QString("InHMD"), controller::StateController::ReadLambda([]() -> float {
        return (float)qApp->getActiveDisplayPlugin()->isHmd();
    }));

    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    userInputMapper->registerDevice(_applicationStateDevice);
    userInputMapper->loadDefaultMapping(userInputMapper->getStandardDeviceID());
}

void PluginApplication::cleanupBeforeQuit() {
    getActiveDisplayPlugin()->deactivate();
    _applicationStateDevice.reset();
}

PluginApplication::~PluginApplication() {
    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        inputPlugin->deactivate();
    }
}

void PluginApplication::initializeGL() {
    _offscreenContext = new OffscreenGLCanvas();
    _offscreenContext->create(getWindow()->context()->getContext());
    _offscreenContext->makeCurrent();
}

void PluginApplication::resizeGL() {
    GLApplication::resizeGL();

    if (!_displayPlugin) {
        return;
    }

    PROFILE_RANGE(__FUNCTION__);
    auto displayPlugin = getActiveDisplayPlugin();
    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    uvec2 framebufferSize = displayPlugin->getRecommendedRenderSize();
    uvec2 renderSize = uvec2(vec2(framebufferSize) * getRenderResolutionScale());
    if (_renderResolution != renderSize) {
        _renderResolution = renderSize;
        _fboCache.setSize(fromGlm(framebufferSize));
    }
}

void PluginApplication::prePaintGL() {
    if (!_displayPlugin) {
        return;
    }

    PROFILE_RANGE(__FUNCTION__);
    auto displayPlugin = getActiveDisplayPlugin();
    // FIXME not needed anymore?
    _offscreenContext->makeCurrent();

    // Primary rendering pass
    if (nullptr == _currentFramebuffer) {
        // Final framebuffer that will be handled to the display-plugin
        _currentFramebuffer = _fboCache.getReadyFbo();
    }

    _currentFramebuffer->bind();
}

void PluginApplication::submitGL() {
    if (!_currentFramebuffer) {
        return;
    }

    QOpenGLFramebufferObject::bindDefault();
    GLuint finalTexture = _currentFramebuffer->texture();
    if (!finalTexture) {
        return;
    }

    PROFILE_RANGE(__FUNCTION__ "/pluginOutput");
    _fboCache.lockTexture(finalTexture);
    _currentFramebuffer = nullptr;
    // deliver final scene to the display plugin
    auto displayPlugin = getActiveDisplayPlugin();

    {
        PROFILE_RANGE(__FUNCTION__ "/pluginSubmitScene");
        const auto size = _fboCache.getSize();
        displayPlugin->submitSceneTexture(getFrameCount(), finalTexture, toGlm(size));
    }
}

void PluginApplication::idle() {
    GLApplication::idle();
    {
        PerformanceTimer perfTimer("pluginIdle");
        getActiveDisplayPlugin()->idle();
        auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
        foreach(auto inputPlugin, inputPlugins) {
            if (inputPlugin->isActive()) {
                inputPlugin->idle();
            }
        }
    }
}

void PluginApplication::update(float deltaTime) {
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    userInputMapper->update(deltaTime);

    bool jointsCaptured = false;
    for (auto inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isActive()) {
            inputPlugin->pluginUpdate(deltaTime, jointsCaptured);
            if (inputPlugin->isJointController()) {
                jointsCaptured = true;
            }
        }
    }

    controller::Pose leftHand = userInputMapper->getPoseState(controller::Action::LEFT_HAND);
    controller::Pose rightHand = userInputMapper->getPoseState(controller::Action::RIGHT_HAND);
    quint64 now = usecTimestampNow();
}

void PluginApplication::resetSensors(bool andReload) {
    getActiveDisplayPlugin()->resetSensors();

    QScreen* currentScreen = getWindow()->screen();
    QPoint windowCenter = getWindow()->geometry().center();
    getWindow()->cursor().setPos(currentScreen, windowCenter);
}

DisplayPluginPointer PluginApplication::getActiveDisplayPlugin() {
    if (nullptr == _displayPlugin) {
        updateDisplayMode();
        Q_ASSERT(_displayPlugin);
    }
    return _displayPlugin;
}

const DisplayPluginPointer PluginApplication::getActiveDisplayPlugin() const {
    return ((PluginApplication*)this)->getActiveDisplayPlugin();
}

static void addDisplayPluginToMenu(DisplayPluginPointer displayPlugin, bool active = false) {
}

void PluginApplication::updateDisplayMode() {
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;
        foreach(auto displayPlugin, displayPlugins) {
            addDisplayPluginToMenu(displayPlugin, first);
            // This must be a queued connection to avoid a deadlock
            QObject::connect(displayPlugin.get(), &DisplayPlugin::requestRender, [this] {
                postEvent(this, new QEvent((QEvent::Type)CustomEventTypes::Render), Qt::HighEventPriority);
            });
            QObject::connect(displayPlugin.get(), &DisplayPlugin::recommendedFramebufferSizeChanged, [this](const QSize & size) {
                resizeGL();
            });

            first = false;
        }
    });


    // Default to the first item on the list, in case none of the menu items match
    static DisplayPluginPointer primaryDisplayPlugin = displayPlugins.at(0);
    
    DisplayPluginPointer newDisplayPlugin = _newDisplayPlugin;
    if (!_newDisplayPlugin) {
        _newDisplayPlugin = primaryDisplayPlugin;
    }

    if (_newDisplayPlugin == _displayPlugin) {
        _newDisplayPlugin.reset();
        return;
    }

    // Some plugins *cough* Oculus *cough* process message events from inside their
    // display function, and we don't want to change the display plugin underneath
    // the paintGL call, so we need to guard against that
    if (_inPaint) {
        qDebug() << "Deferring plugin switch until out of painting";
        // Have the old plugin stop requesting renders
        _displayPlugin->stop();
        QTimer* timer = new QTimer();
        timer->singleShot(500, [this, timer] {
            timer->deleteLater();
            updateDisplayMode();
        });
        return;
    }

    if (_newDisplayPlugin) {
        _newDisplayPlugin->activate();
        _offscreenContext->makeCurrent();
    }

    DisplayPluginPointer oldDisplayPlugin = _displayPlugin;
    _displayPlugin = _newDisplayPlugin;
    _newDisplayPlugin.reset();

    if (oldDisplayPlugin) {
        oldDisplayPlugin->deactivate();
    }

    emit activeDisplayPluginChanged();
    resetSensors();
    Q_ASSERT_X(_displayPlugin, "PluginApplication::updateDisplayMode", "could not find an activated display plugin");
}

static void addInputPluginToMenu(InputPluginPointer inputPlugin, bool active = false) {
}


void PluginApplication::updateInputModes() {
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;
        foreach(auto inputPlugin, inputPlugins) {
            addInputPluginToMenu(inputPlugin, first);
            first = false;
        }
    });

    InputPluginList newInputPlugins;
    InputPluginList removedInputPlugins;
    foreach(auto inputPlugin, inputPlugins) {
        //QString name = inputPlugin->getName();
        //QAction* action = menu->getActionForOption(name);

        //auto it = std::find(std::begin(_activeInputPlugins), std::end(_activeInputPlugins), inputPlugin);
        //if (action->isChecked() && it == std::end(_activeInputPlugins)) {
        //    _activeInputPlugins.push_back(inputPlugin);
        //    newInputPlugins.push_back(inputPlugin);
        //} else if (!action->isChecked() && it != std::end(_activeInputPlugins)) {
        //    _activeInputPlugins.erase(it);
        //    removedInputPlugins.push_back(inputPlugin);
        //}
    }

    // A plugin was checked
    if (newInputPlugins.size() > 0) {
        foreach(auto newInputPlugin, newInputPlugins) {
            newInputPlugin->activate();
        }
    }
    if (removedInputPlugins.size() > 0) { // A plugin was unchecked
        foreach(auto removedInputPlugin, removedInputPlugins) {
            removedInputPlugin->deactivate();
        }
    }
}

void PluginApplication::setActiveDisplayPlugin(const QString& pluginName) {
    foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
        QString name = displayPlugin->getName();
        if (pluginName == name) {
            _newDisplayPlugin = displayPlugin;
            break;
        }
    }
    updateDisplayMode();
}

void PluginApplication::releaseSceneTexture(uint32_t texture) {
    _fboCache.releaseTexture(texture);
}

void PluginApplication::makeRenderingContextCurrent() {
    _offscreenContext->makeCurrent();
}

float PluginApplication::getRenderResolutionScale() const {
    return 1.0f;
}

void PluginApplication::setFullscreen(QScreen* screen) {
}

void PluginApplication::unsetFullscreen() {
}
