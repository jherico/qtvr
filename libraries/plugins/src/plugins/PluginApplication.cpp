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
#include <NumericalConstants.h>

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
#include <OffscreenUi.h>

static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;
static const unsigned int CAPPED_SIM_FRAMERATE = 60;
static const int CAPPED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / CAPPED_SIM_FRAMERATE;


PluginApplication::PluginApplication(int& argc, char** argv)
    : UiApplication(argc, argv) {

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
    UiApplication::resizeGL();

    if (!_displayPlugin) {
        return;
    }

    resizeUi(_displayPlugin->getRecommendedUiSize());

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

    mat4 headPose = displayPlugin->getHeadPose(getFrameCount());
    {
        // FIXME we probably don't need to set the projection matrix every frame,
        // only when the display plugin changes (or in non-HMD modes when the user
        // changes the FOV manually, which right now I don't think they can.
        for_each_eye([&](Eye eye) {
            // For providing the stereo eye views, the HMD head pose has already been
            // applied to the avatar, so we need to get the difference between the head
            // pose applied to the avatar and the per eye pose, and use THAT as
            // the per-eye stereo matrix adjustment.
            mat4 eyeToHead = displayPlugin->getEyeToHeadTransform(eye);
            // Grab the translation
            vec3 eyeOffset = glm::vec3(eyeToHead[3]);
            // Apply IPD scaling
            mat4 eyeOffsetTransform = glm::translate(mat4(), eyeOffset * -1.0f);
            displayPlugin->setEyeRenderPose(getFrameCount(), eye, headPose);
        });


        PROFILE_RANGE(__FUNCTION__ "/pluginSubmitScene");
        const auto size = _fboCache.getSize();
        displayPlugin->submitSceneTexture(getFrameCount(), finalTexture, toGlm(size));
    }
    _pendingPaint = false;
}

static const uint32_t INVALID_FRAME = UINT32_MAX;
static uint32_t _renderedFrameIndex { INVALID_FRAME };

void PluginApplication::idle() {
    auto now = usecTimestampNow();
    auto displayPlugin = getActiveDisplayPlugin();
    // depending on whether we're throttling or not.
    // Once rendering is off on another thread we should be able to have Application::idle run at start(0) in
    // perpetuity and not expect events to get backed up.
    bool isThrottled = displayPlugin->isThrottled();
    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    // This attempts to lock the simulation at 60 updates per second, regardless of framerate
    float timeSinceLastUpdateUs = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC;
    float secondsSinceLastUpdate = timeSinceLastUpdateUs / USECS_PER_SECOND;

    auto presentCount = displayPlugin->presentCount();
    if (presentCount < _renderedFrameIndex) {
        _renderedFrameIndex = INVALID_FRAME;
    }

    // Nested ifs are for clarity in the logic.  Don't collapse them into a giant single if.
    // Don't saturate the main thread with rendering, no paint calls until the last one is complete
    if (!_pendingPaint) {
        // Also no paint calls until the display plugin has increased by at least one frame 
        // (don't render at 90fps if the display plugin only goes at 60)
        if (_renderedFrameIndex == INVALID_FRAME || presentCount > _renderedFrameIndex) {
            // Record what present frame we're on
            _renderedFrameIndex = presentCount;
            // Don't allow paint requests to stack up in the event queue
            _pendingPaint = true;
            // But when we DO request a paint, get to it as soon as possible: high priority
            postEvent(this, new QEvent(static_cast<QEvent::Type>(Render)), Qt::HighEventPriority);
        }
    }

    // For the rest of idle, we want to cap at the max sim rate, so we might not call 
    // the remaining idle work every paint frame, or vice versa
    // In theory this means we could call idle processing more often than painting,
    // but in practice, when the paintGL calls aren't keeping up, there's no room left
    // in the main thread to call idle more often than paint.
    // This check is mostly to keep idle from burning up CPU cycles by running at 
    // hundreds of idles per second when the rendering is that fast
    if ((timeSinceLastUpdateUs / USECS_PER_MSEC) < CAPPED_SIM_FRAME_PERIOD_MS) {
        // No paint this round, but might be time for a new idle, otherwise return
        return;
    }

    // We're going to execute idle processing, so restart the last idle timer
    _lastTimeUpdated.start();

    UiApplication::idle();
    return;
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

void PluginApplication::updateDisplayMode() {
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
    // Default to the first item on the list, in case none of the menu items match
    static DisplayPluginPointer primaryDisplayPlugin = displayPlugins.at(0);
   
    if (!_newDisplayPlugin) {
        _newDisplayPlugin = primaryDisplayPlugin;
    }

    if (_newDisplayPlugin == _displayPlugin) {
        _newDisplayPlugin.reset();
        return;
    }


    DisplayPluginPointer newDisplayPlugin = _newDisplayPlugin;

    // Some plugins *cough* Oculus *cough* process message events from inside their
    // display function, and we don't want to change the display plugin underneath
    // the paintGL call, so we need to guard against that
    if (_inPaint) {
        qDebug() << "Deferring plugin switch until out of painting";
        // Have the old plugin stop requesting renders
        if (_displayPlugin) {
            _displayPlugin->stop();
        }
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

    static std::once_flag once;
    std::call_once(once, [&] {
        QStringList plugins;
        QString activePlugin;
        foreach(auto displayPlugin, displayPlugins) {
            plugins << displayPlugin->getName();

            if (displayPlugin->isActive()) {
                activePlugin = displayPlugin->getName();
            }

            QObject::connect(displayPlugin.get(), &DisplayPlugin::recommendedFramebufferSizeChanged, [this](const QSize & size) {
                resizeGL();
            });
        }
        qDebug() << plugins;
        auto menu = getOffscreenUi()->getMenu();
        QObject::connect(menu, SIGNAL(activateDisplayPlugin(QString)), this, SLOT(setActiveDisplayPlugin(QString)));
        QMetaObject::invokeMethod(menu, "setDisplayPlugins", Q_ARG(QVariant, plugins), Q_ARG(QVariant, activePlugin));
    });


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

void PluginApplication::setActiveDisplayPlugin(QString pluginName) {
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


void PluginApplication::releaseOverlayTexture(uint32_t texture) {
    _offscreenUi->releaseTexture(texture);
}

float PluginApplication::getRenderResolutionScale() const {
    return 1.0f;
}

void PluginApplication::setFullscreen(QScreen* screen) {
}

void PluginApplication::unsetFullscreen() {
}

void PluginApplication::updateOverlayTexture(uint32_t textureId, const glm::uvec2& size) {
    UiApplication::updateOverlayTexture(textureId, size);
    auto displayPlugin = getActiveDisplayPlugin();
    if (displayPlugin) {
        displayPlugin->submitOverlayTexture(textureId, size);
    }
}

void PluginApplication::initializeUI(const QUrl& desktopUrl) {
    UiApplication::initializeUI(desktopUrl);
    // This will set up the input plugins UI
    _activeInputPlugins.clear();
    updateInputModes();
}

QOpenGLContext* PluginApplication::getPrimaryRenderingContext() {
    return _offscreenContext->getContext();
}

bool PluginApplication::makePrimaryRenderingContextCurrent() {
    return _offscreenContext->makeCurrent();
}

void PluginApplication::restoreDefaultFramebuffer() {
    _currentFramebuffer->bind();
}
