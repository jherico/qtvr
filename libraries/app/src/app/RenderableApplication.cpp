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

#include "RenderableApplication.h"

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

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickWindow>
#include <QtNetwork/QNetworkDiskCache>

#include <gl/Config.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLWindow.h>
#include <gl/OffscreenGLCanvas.h>

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/GLBackend.h>
#include <gpu/FramebufferCache.h>


#include <plugins/DisplayPlugin.h>
#include <plugins/InputPlugin.h>
#include <plugins/PluginContainer.h>
#include <plugins/PluginManager.h>

#include <controllers/UserInputMapper.h>
#include <controllers/StateController.h>

#include <Menu.h>

#include <SettingManager.h>
#include <SettingHandle.h>
#include <Finally.h>
#include <LogHandler.h>
#include <MessageDialog.h>
#include <OctalCode.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <ResourceCache.h>
#include <UUID.h>
#include <VrMenu.h>

#include "PluginContainerProxy.h"

//Q_DECLARE_LOGGING_CATEGORY(interfaceapp)
//Q_DECLARE_LOGGING_CATEGORY(interfaceapp_timing)

Q_LOGGING_CATEGORY(interfaceapp, "hifi.interface")
Q_LOGGING_CATEGORY(interfaceapp_timing, "hifi.interface.timing")

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

// ON WIndows PC, NVidia Optimus laptop, we want to enable NVIDIA GPU
// FIXME seems to be broken.
#if defined(Q_OS_WIN)
extern "C" {
 _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

using namespace std;

static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;

static const float PHYSICS_READY_RANGE = 3.0f; // how far from avatar to check for entities that aren't ready for simulation

#ifndef __APPLE__
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/script.js");
#endif

enum CustomEventTypes {
    Lambda = QEvent::User + 1,
    Render = QEvent::User + 2
};

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
    QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
    QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    void call() { _fun(); }
};

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        qApp->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

bool setupEssentials(int& argc, char** argv) {
    // Set build version
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    Setting::preInit();

    Setting::init();

    // Set dependencies
    DependencyManager::set<FramebufferCache>();
    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<OffscreenUi>();
    DependencyManager::set<PathUtils>();
    DependencyManager::set<UserInputMapper>();
    return true;
}


// FIXME hack access to the internal share context for the Chromium helper
// Normally we'd want to use QWebEngine::initialize(), but we can't because 
// our primary context is a QGLWidget, which can't easily be initialized to share
// from a QOpenGLContext.
//
// So instead we create a new offscreen context to share with the QGLWidget,
// and manually set THAT to be the shared context for the Chromium helper
OffscreenGLCanvas* _chromiumShareContext { nullptr };
Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);

RenderableApplication::RenderableApplication(int& argc, char** argv, QElapsedTimer& startupTimer) 
    : QGuiApplication(argc, argv) {
    setupEssentials(argc, argv);
    thread()->setObjectName("Main Thread");

    // to work around the Qt constant wireless scanning, set the env for polling interval very high
    const QByteArray EXTREME_BEARER_POLL_TIMEOUT = QString::number(INT_MAX).toLocal8Bit();
    qputenv("QT_BEARER_POLL_TIMEOUT", EXTREME_BEARER_POLL_TIMEOUT);

    _pluginContainer = new PluginContainerProxy();
    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);


    qDebug() << "[VERSION] Build sequence:" << qPrintable(applicationVersion());

    ResourceCache::setRequestLimit(3);

    _window = new GLWindow();
    _window->setCursor(Qt::BlankCursor);
    _window->createContext();
    _window->setVisible(true);
    _window->setTitle("Test");

    // enable mouse tracking; otherwise, we only get drag events
    _window->setMouseGrabEnabled(true);
    _window->makeCurrent();

    _chromiumShareContext = new OffscreenGLCanvas();
    _chromiumShareContext->create(_window->context()->getContext());
    _chromiumShareContext->makeCurrent();
    qt_gl_set_global_share_context(_chromiumShareContext->getContext());

    _offscreenContext = new OffscreenGLCanvas();
    _offscreenContext->create(_window->context()->getContext());
    _offscreenContext->makeCurrent();
    initializeGL();

    _offscreenContext->makeCurrent();

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    // Setup the userInputMapper with the actions
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, [this](int action, float state) {
        using namespace controller;
        static auto offscreenUi = DependencyManager::get<OffscreenUi>();
        if (offscreenUi->navigationFocused()) {
            auto actionEnum = static_cast<Action>(action);
            int key = Qt::Key_unknown;
            static int lastKey = Qt::Key_unknown;
            bool navAxis = false;
            switch (actionEnum) {
                case Action::UI_NAV_VERTICAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Up;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Down;
                    }
                    break;

                case Action::UI_NAV_LATERAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Right;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Left;
                    }
                    break;

                case Action::UI_NAV_GROUP:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Tab;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Backtab;
                    }
                    break;

                case Action::UI_NAV_BACK:
                    key = Qt::Key_Escape;
                    break;

                case Action::UI_NAV_SELECT:
                    key = Qt::Key_Return;
                    break;
            }

            if (navAxis) {
                if (lastKey != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyRelease, lastKey, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                    lastKey = Qt::Key_unknown;
                }

                if (key != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                    lastKey = key;
                }
            } else if (key != Qt::Key_unknown) {
                if (state) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                } else {
                    QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                }
                return;
            }
        }

        if (action == controller::toInt(controller::Action::RETICLE_CLICK)) {
            auto globalPos = QCursor::pos();
            auto localPos = _window->mapFromGlobal(globalPos);
            if (state) {
                QMouseEvent mousePress(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                sendEvent(_window, &mousePress);
                _reticleClickPressed = true;
            } else {
                QMouseEvent mouseRelease(QEvent::MouseButtonRelease, localPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                sendEvent(_window, &mouseRelease);
                _reticleClickPressed = false;
            }
            return; // nothing else to do
        }

        if (state) {
            if (action == controller::toInt(controller::Action::TOGGLE_MUTE)) {
            } else if (action == controller::toInt(controller::Action::CYCLE_CAMERA)) {
            } else if (action == controller::toInt(controller::Action::CONTEXT_MENU)) {
            } else if (action == controller::toInt(controller::Action::RETICLE_X)) {
                auto oldPos = QCursor::pos();
                auto newPos = oldPos;
                newPos.setX(oldPos.x() + state);
                QCursor::setPos(newPos);


                // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
                // remove it after we're done
                const float REASONABLE_CHANGE = 50.0f;
                glm::vec2 oldPosG = { oldPos.x(), oldPos.y() };
                glm::vec2 newPosG = { newPos.x(), newPos.y() };
                auto distance = glm::distance(oldPosG, newPosG);
                if (distance > REASONABLE_CHANGE) {
//                    qDebug() << "Action::RETICLE_X... UNREASONABLE CHANGE! distance:" << distance << " oldPos:" << oldPosG << " newPos:" << newPosG;
                }

            } else if (action == controller::toInt(controller::Action::RETICLE_Y)) {
                auto oldPos = QCursor::pos();
                auto newPos = oldPos;
                newPos.setY(oldPos.y() + state);
                QCursor::setPos(newPos);

                // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
                // remove it after we're done
                const float REASONABLE_CHANGE = 50.0f;
                glm::vec2 oldPosG = { oldPos.x(), oldPos.y() };
                glm::vec2 newPosG = { newPos.x(), newPos.y() };
                auto distance = glm::distance(oldPosG, newPosG);
                if (distance > REASONABLE_CHANGE) {
//                    qDebug() << "Action::RETICLE_Y... UNREASONABLE CHANGE! distance:" << distance << " oldPos:" << oldPosG << " newPos:" << newPosG;
                }
            }
        }
    });

    // A new controllerInput device used to reflect current values from the application state
    _applicationStateDevice = std::make_shared<controller::StateController>();

    _applicationStateDevice->addInputVariant(QString("InHMD"), controller::StateController::ReadLambda([]() -> float {
        return (float)qApp->getActiveDisplayPlugin()->isHmd();
    }));
    _applicationStateDevice->addInputVariant(QString("NavigationFocused"), controller::StateController::ReadLambda([]() -> float {
        static auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->navigationFocused() ? 1.0 : 0.0;
    }));
    userInputMapper->registerDevice(_applicationStateDevice);
    userInputMapper->loadDefaultMapping(userInputMapper->getStandardDeviceID());
    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now

    connect(this, &RenderableApplication::applicationStateChanged, this, &RenderableApplication::activeChanged);

    _idleTimer.setInterval(0);
    connect(&_idleTimer, &QTimer::timeout, [this] {
        idle();
    });
    _idleTimer.start();
    qCDebug(interfaceapp, "Startup time: %4.2f seconds.", (double)startupTimer.elapsed() / 1000.0);
}

void RenderableApplication::aboutToQuit() {
    emit beforeAboutToQuit();

    getActiveDisplayPlugin()->deactivate();

    _aboutToQuit = true;

    cleanupBeforeQuit();
}

void RenderableApplication::cleanupBeforeQuit() {
    // FIXME: once we move to shared pointer for the INputDevice we shoud remove this naked delete:
    _applicationStateDevice.reset();
//    _window->saveGeometry();
}

RenderableApplication::~RenderableApplication() {
    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        inputPlugin->deactivate();
    }
    DependencyManager::destroy<OffscreenUi>();
    DependencyManager::destroy<FramebufferCache>();
    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

void RenderableApplication::initializeGL() {
    qCDebug(interfaceapp) << "Created Display Window.";

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    if (_isGLInitialized) {
        return;
    } else {
        _isGLInitialized = true;
    }

    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    gpu::Context::init<gpu::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();

    // The UI can't be created until the primary OpenGL
    // context is created, because it needs to share
    // texture resources
    initializeUi();
    qCDebug(interfaceapp, "Initialized Offscreen UI.");
    _offscreenContext->makeCurrent();

    _idleLoopStdev.reset();

    // update before the first render
    update(1.0f / _fps);
}

void RenderableApplication::initializeUi() {
    MessageDialog::registerType();
    VrMenu::registerType();

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->create(_offscreenContext->getContext());
    offscreenUi->setProxyWindow(_window);
    offscreenUi->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    offscreenUi->load("Root.qml");
    offscreenUi->load("RootMenu.qml");

    auto rootContext = offscreenUi->getRootContext();
    auto engine = rootContext->engine();
    connect(engine, &QQmlEngine::quit, [] {
        qApp->quit();
    });

    rootContext->setContextProperty("Paths", DependencyManager::get<PathUtils>().data());
    _window->installEventFilter(offscreenUi.data());

    connect(offscreenUi.data(), &OffscreenUi::textureUpdated, this, [this, offscreenUi](GLuint textureId) {
        _uiTexture = textureId;
        offscreenUi->lockTexture(_uiTexture);
        auto displayPlugin = getActiveDisplayPlugin();
        if (displayPlugin) {
            displayPlugin->submitOverlayTexture(_uiTexture, uvec2());
        }
    });

    VrMenu::load();
    VrMenu::executeQueuedLambdas();
    offscreenUi->resume();

    // This will set up the input plugins UI
    _activeInputPlugins.clear();
    updateInputModes();
}

void RenderableApplication::paintGL() {
    // paintGL uses a queued connection, so we can get messages from the queue even after we've quit 
    // and the plugins have shutdown
    if (_aboutToQuit) {
        return;
    }

    if (!_displayPlugin) {
        return;
    }

    // Some plugins process message events, potentially leading to
    // re-entering a paint event.  don't allow further processing if this
    // happens
    if (_inPaint) {
        return;
    }
    _inPaint = true;

    _frameCount++;

    // update fps moving average
    uint64_t now = usecTimestampNow();
    static uint64_t lastPaintBegin{ now };
    uint64_t diff = now - lastPaintBegin;
    float instantaneousFps = 0.0f;
    if (diff != 0) {
        instantaneousFps = (float)USECS_PER_SECOND / (float)diff;
        _framesPerSecond.updateAverage(_lastInstantaneousFps);
    }

    lastPaintBegin = now;

    // update fps once a second
    if (now - _lastFramesPerSecondUpdate > USECS_PER_SECOND) {
        _fps = _framesPerSecond.getAverage();
        _lastFramesPerSecondUpdate = now;
    }

    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("paintGL");

    Finally clearFlagLambda([this] { _inPaint = false; });

    auto displayPlugin = getActiveDisplayPlugin();
    // FIXME not needed anymore?
    _offscreenContext->makeCurrent();

    resizeGL();

    // Before anything else, let's sync up the gpuContext with the true glcontext used in case anything happened
    {
        PerformanceTimer perfTimer("syncCache");
        _gpuContext->syncCache();
    }

    // Primary rendering pass
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    const auto size = framebufferCache->getFrameBufferSize();
    // Final framebuffer that will be handled to the display-plugin
    auto finalFramebuffer = framebufferCache->getFramebuffer();

    {
        PROFILE_RANGE(__FUNCTION__ "/mainRender");
        PerformanceTimer perfTimer("mainRender");
        gpu::doInBatch(_gpuContext, [=](gpu::Batch& batch) { 
            batch.setFramebuffer(finalFramebuffer);
            batch.setViewportTransform(ivec4(0, 0, size.x, size.y));
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
        });
    }

    // deliver final scene to the display plugin
    {
        PROFILE_RANGE(__FUNCTION__ "/pluginOutput");
        PerformanceTimer perfTimer("pluginOutput");

        auto finalTexturePointer = finalFramebuffer->getRenderBuffer(0);
        GLuint finalTexture = gpu::GLBackend::getTextureID(finalTexturePointer);
        Q_ASSERT(0 != finalTexture);
        Q_ASSERT(!_lockedFramebufferMap.contains(finalTexture));
        _lockedFramebufferMap[finalTexture] = finalFramebuffer;

        {
            PROFILE_RANGE(__FUNCTION__ "/pluginSubmitScene");
            PerformanceTimer perfTimer("pluginSubmitScene");
            displayPlugin->submitSceneTexture(_frameCount, finalTexture, size);
        }
    }

    {
        // Reset the gpu::Context Stages
        // Back to the default framebuffer;
        gpu::doInBatch(_gpuContext, [=](gpu::Batch& batch) { batch.resetStages(); });
    }
}


void RenderableApplication::aboutApp() {
}

void RenderableApplication::resizeGL() {
    PROFILE_RANGE(__FUNCTION__);
    if (nullptr == _displayPlugin) {
        return;
    }

    auto displayPlugin = getActiveDisplayPlugin();
    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    uvec2 framebufferSize = displayPlugin->getRecommendedRenderSize();
    uvec2 renderSize = uvec2(vec2(framebufferSize) * getRenderResolutionScale());
    if (_renderResolution != renderSize) {
        _renderResolution = renderSize;
        DependencyManager::get<FramebufferCache>()->setFrameBufferSize(renderSize);
    }

    // FIXME the aspect ratio for stereo displays is incorrect based on this.
    float aspectRatio = displayPlugin->getRecommendedAspectRatio();
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto uiSize = displayPlugin->getRecommendedUiSize();
    // Bit of a hack since there's no device pixel ratio change event I can find.
    static qreal lastDevicePixelRatio = 0;
    qreal devicePixelRatio = _window->devicePixelRatio();
    if (offscreenUi->size() != fromGlm(uiSize) || devicePixelRatio != lastDevicePixelRatio) {
        offscreenUi->resize(fromGlm(uiSize));
        _offscreenContext->makeCurrent();
        lastDevicePixelRatio = devicePixelRatio;
    }
}

bool RenderableApplication::event(QEvent* event) {
    if ((int)event->type() == (int)Lambda) {
        ((LambdaEvent*)event)->call();
        return true;
    }

    if ((int)event->type() == (int)Render) {
        paintGL();
    }

    // handle custom URL
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);

        QUrl url = fileEvent->url();

        if (!url.isEmpty()) {
            QString urlString = url.toString();
            if (canAcceptURL(urlString)) {
                return acceptURL(urlString);
            }
        }
        return false;
    }

    return QGuiApplication::event(event);
}

//void RenderableApplication::focusOutEvent(QFocusEvent* event) {
//    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
//    foreach(auto inputPlugin, inputPlugins) {
//        if (inputPlugin->isActive()) {
//            inputPlugin->pluginFocusOutEvent();
//        }
//    }
//
//    // synthesize events for keys currently pressed, since we may not get their release events
//    foreach (int key, _keysPressed) {
//        QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
//        keyReleaseEvent(&event);
//    }
//
//    _keysPressed.clear();
//}

void RenderableApplication::idle() {
    uint64_t now = usecTimestampNow();
    if (_aboutToQuit) {
        return; // bail early, nothing to do here.
    }

    // depending on whether we're throttling or not.
    // Once rendering is off on another thread we should be able to have RenderableApplication::idle run at start(0) in
    // perpetuity and not expect events to get backed up.
    bool isThrottled = getActiveDisplayPlugin()->isThrottled();
    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    // This attempts to lock the simulation at 60 updates per second, regardless of framerate
    float timeSinceLastUpdateUs = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC;
    float secondsSinceLastUpdate = timeSinceLastUpdateUs / USECS_PER_SECOND;

    if (isThrottled && (timeSinceLastUpdateUs / USECS_PER_MSEC) < THROTTLED_SIM_FRAME_PERIOD_MS) {
        return; // bail early, we're throttled and not enough time has elapsed
    }

    _lastTimeUpdated.start();


    {
        PROFILE_RANGE(__FUNCTION__);
        static uint64_t lastIdleStart{ now };
        uint64_t idleStartToStartDuration = now - lastIdleStart;
        if (idleStartToStartDuration != 0) {
            _simsPerSecond.updateAverage((float)USECS_PER_SECOND / (float)idleStartToStartDuration);
        }
        lastIdleStart = now;
    }

    PerformanceTimer perfTimer("idle");

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and its subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    {
        PerformanceTimer perfTimer("update");
        PerformanceWarning warn(showWarnings, "RenderableApplication::idle()... update()");
        static const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp(secondsSinceLastUpdate, 0.0f, BIGGEST_DELTA_TIME_SECS));
    }
    {
        PerformanceTimer perfTimer("pluginIdle");
        PerformanceWarning warn(showWarnings, "RenderableApplication::idle()... pluginIdle()");
        getActiveDisplayPlugin()->idle();
        auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
        foreach(auto inputPlugin, inputPlugins) {
            if (inputPlugin->isActive()) {
                inputPlugin->idle();
            }
        }
    }
    {
        PerformanceTimer perfTimer("rest");
        PerformanceWarning warn(showWarnings, "RenderableApplication::idle()... rest of it");
        _idleLoopStdev.addValue(secondsSinceLastUpdate);

        //  Record standard deviation and reset counter if needed
        const int STDEV_SAMPLES = 500;
        if (_idleLoopStdev.getSamples() > STDEV_SAMPLES) {
            _idleLoopMeasuredJitter = _idleLoopStdev.getStDev();
            _idleLoopStdev.reset();
        }
    }
}

float RenderableApplication::getAverageSimsPerSecond() {
    uint64_t now = usecTimestampNow();

    if (now - _lastSimsPerSecondUpdate > USECS_PER_SECOND) {
        _simsPerSecondReport = _simsPerSecond.getAverage();
        _lastSimsPerSecondUpdate = now;
    }
    return _simsPerSecondReport;
}

ivec2 RenderableApplication::getMouse() const {
    return getTrueMouse();
}

void RenderableApplication::update(float deltaTime) {
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


void RenderableApplication::resetSensors(bool andReload) {
    getActiveDisplayPlugin()->resetSensors();

    QScreen* currentScreen = _window->screen();
    QPoint windowCenter = _window->geometry().center();
    _window->cursor().setPos(currentScreen, windowCenter);
}

void RenderableApplication::activeChanged(Qt::ApplicationState state) {
    switch (state) {
        case Qt::ApplicationActive:
            _isForeground = true;
            break;

        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
        default:
            _isForeground = false;
            break;
    }
}

void RenderableApplication::postLambdaEvent(std::function<void()> f) {
    if (this->thread() == QThread::currentThread()) {
        f();
    } else {
        QCoreApplication::postEvent(this, new LambdaEvent(f));
    }
}

uvec2 RenderableApplication::getCanvasSize() const {
    return glm::uvec2(_window->width(), _window->height());
}

uvec2 RenderableApplication::getUiSize() const {
    return getActiveDisplayPlugin()->getRecommendedUiSize();
}

uvec2 RenderableApplication::getDeviceSize() const {
    return getActiveDisplayPlugin()->getRecommendedRenderSize();
}

bool RenderableApplication::isThrottleRendering() const {
    return getActiveDisplayPlugin()->isThrottled();
}

ivec2 RenderableApplication::getTrueMouse() const {
    return toGlm(_window->mapFromGlobal(QCursor::pos()));
}

bool RenderableApplication::hasFocus() const {
    return getActiveDisplayPlugin()->hasFocus();
}

uvec2 RenderableApplication::getViewportDimensions() const {
    return getDeviceSize();
}

DisplayPlugin* RenderableApplication::getActiveDisplayPlugin() {
    if (nullptr == _displayPlugin) {
        updateDisplayMode();
        Q_ASSERT(_displayPlugin);
    }
    return _displayPlugin.get();
}

const DisplayPlugin* RenderableApplication::getActiveDisplayPlugin() const {
    return ((RenderableApplication*)this)->getActiveDisplayPlugin();
}

static void addDisplayPluginToMenu(DisplayPluginPointer displayPlugin, bool active = false) {
}

void RenderableApplication::updateDisplayMode() {
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

    _pluginContainer->clearDisplayPluginItems();

    if (_newDisplayPlugin) {
        _newDisplayPlugin->activate();
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->resize(fromGlm(_newDisplayPlugin->getRecommendedUiSize()));
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
    Q_ASSERT_X(_displayPlugin, "RenderableApplication::updateDisplayMode", "could not find an activated display plugin");
}

static void addInputPluginToMenu(InputPluginPointer inputPlugin, bool active = false) {

}


void RenderableApplication::updateInputModes() {
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;
        foreach(auto inputPlugin, inputPlugins) {
            addInputPluginToMenu(inputPlugin, first);
            first = false;
        }
    });
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

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

mat4 RenderableApplication::getEyeProjection(int eye) const {
    auto displayPlugin = getActiveDisplayPlugin();
    if (displayPlugin->isHmd()) {
        return displayPlugin->getProjection((Eye)eye, _viewFrustum.getProjection());
    }

    return _viewFrustum.getProjection();
}

mat4 RenderableApplication::getEyeOffset(int eye) const {
    // FIXME invert?
    return getActiveDisplayPlugin()->getEyeToHeadTransform((Eye)eye);
}

mat4 RenderableApplication::getHMDSensorPose() const {
    auto displayPlugin = getActiveDisplayPlugin();
    if (displayPlugin->isHmd()) {
        return displayPlugin->getHeadPose(_frameCount);
    }
    return mat4();
}

void RenderableApplication::crashApplication() {
    qCDebug(interfaceapp) << "Intentionally crashed Interface";
    int* value = nullptr;
    *value = 1;
}

void RenderableApplication::setActiveDisplayPlugin(const QString& pluginName) {
    foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
        QString name = displayPlugin->getName();
        if (pluginName == name) {
            _newDisplayPlugin = displayPlugin;
            break;
        }
    }
    updateDisplayMode();
}

GLWindow* RenderableApplication::getWindow() {
    return _window;
}

float RenderableApplication::getRenderResolutionScale() const {
    return _resolutionScale;
}

