//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderApplication_h
#define hifi_RenderApplication_h

#include <functional>

#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtCore/QElapsedTimer>

#include <QtGui/qevent.h>
#include <QtGui/QImage>
#include <QtGui/QGuiApplication>

#include <plugins/Forward.h>
#include <FileLogger.h>
#include <gpu/Context.h>
#include <StDev.h>
#include <ViewFrustum.h>
#include <SimpleMovingAverage.h>

class GLWindow;
class OffscreenGLCanvas;

namespace controller {
    class StateController;
}

class RenderableApplication : public QGuiApplication  {
    Q_OBJECT
    
    friend class PluginContainerProxy;

public:
    RenderableApplication(int& argc, char** argv, QElapsedTimer& startup_time);
    virtual ~RenderableApplication();

    void postLambdaEvent(std::function<void()> f);

    void initializeGL();
    void initializeUi();
    void paintGL();
    void resizeGL();

    bool event(QEvent* event);
    //bool eventFilter(QObject* object, QEvent* event);

    uvec2 getCanvasSize() const;
    uvec2 getUiSize() const;
    uvec2 getDeviceSize() const;
    uvec2 getViewportDimensions() const;
    bool hasFocus() const;
    bool isForeground() const { return _isForeground; }
    bool isThrottleRendering() const;

    GLWindow* getWindow();
    const GLWindow* getWindow() const;

    ivec2 getMouse() const;
    ivec2 getTrueMouse() const;
    uint32_t getFrameCount() { return _frameCount; }
    float getFps() const { return _fps; }
    float getTargetFrameRate(); // frames/second
    float getLastInstanteousFps() const { return _lastInstantaneousFps; }
    float getLastUnsynchronizedFps() const { return _lastUnsynchronizedFps; }

    void setActiveDisplayPlugin(const QString& pluginName);

    DisplayPlugin* getActiveDisplayPlugin();
    const DisplayPlugin* getActiveDisplayPlugin() const;

    FileLogger* getLogger() { return _logger; }

    float getRenderResolutionScale() const;

    bool isAboutToQuit() const { return _aboutToQuit; }

    mat4 getHMDSensorPose() const;
    mat4 getEyeOffset(int eye) const;
    mat4 getEyeProjection(int eye) const;

    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

    float getAverageSimsPerSecond();
    virtual bool canAcceptURL(const QString& url) const { return false; }
    virtual bool acceptURL(const QString& url, bool defaultUpload = false) { return false; }

signals:
    void beforeAboutToQuit();
    void activeDisplayPluginChanged();

public slots:
    void resetSensors(bool andReload = false);
    void aboutApp();
    void crashApplication();

private slots:
    void idle();
    void aboutToQuit();
    void activeChanged(Qt::ApplicationState state);
    void updateDisplayMode();
    void updateInputModes();
    
private:
    void init();
    void cleanupBeforeQuit();
    void update(float deltaTime);

    OffscreenGLCanvas* _offscreenContext { nullptr };
    GLWindow* _window { nullptr };
    DisplayPluginPointer _displayPlugin;
    InputPluginList _activeInputPlugins;
    bool _activatingDisplayPlugin { false };
    QMap<uint32_t, gpu::FramebufferPointer> _lockedFramebufferMap;

    // Frame Rate Measurement
    int _frameCount { 0 };
    float _fps { 0 };
    float _resolutionScale { 1.0f };
    QElapsedTimer _timerStart;
    QElapsedTimer _lastTimeUpdated;
    float _lastInstantaneousFps { 0.0f };
    float _lastUnsynchronizedFps { 0.0f };

    std::shared_ptr<controller::StateController> _applicationStateDevice; // Default ApplicationDevice reflecting the state of different properties of the session
//    std::shared_ptr<KeyboardMouseDevice> _keyboardMouseDevice;   // Default input device, the good old keyboard mouse and maybe touchpad
    QSet<int> _keysPressed;
    quint64 _lastAcceptedKeyPress = 0;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;
    bool _aboutToQuit { false };

    FileLogger* _logger;

    uvec2 _renderResolution;
    gpu::ContextPointer _gpuContext; // initialized during window creation

    PluginContainer* _pluginContainer { nullptr };
    DisplayPluginPointer _newDisplayPlugin;
    ViewFrustum _viewFrustum;
    SimpleMovingAverage _framesPerSecond{10};
    quint64 _lastFramesPerSecondUpdate = 0;
    SimpleMovingAverage _simsPerSecond{10};
    int _simsPerSecondReport = 0;
    quint64 _lastSimsPerSecondUpdate = 0;
    uint32_t _uiTexture;
    QTimer _idleTimer;
    bool _isForeground { true }; // starts out assumed to be in foreground
    bool _inPaint { false };
    bool _isGLInitialized { false };
    bool _physicsEnabled { false };
    bool _reticleClickPressed { false };
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<RenderableApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
