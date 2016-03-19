//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PluginApplication_h
#define hifi_PluginApplication_h

#include <gl/GLApplication.h>

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

#include "Forward.h"

#include <gl/FboCache.h>
#include <UiApplication.h>

class OffscreenGLCanvas;

namespace controller {
    class StateController;
}

class PluginApplication : public UiApplication  {
    Q_OBJECT
    
public:
    PluginApplication(int& argc, char** argv);
    virtual ~PluginApplication();

    virtual void initializeGL() override;
    virtual void prePaintGL() override;
    virtual void resizeGL() override;
    virtual void submitGL() override;

    ivec2 getMouse() const;
    ivec2 getTrueMouse() const;

    float getTargetFrameRate(); // frames/second

    DisplayPluginPointer getActiveDisplayPlugin();
    const DisplayPluginPointer getActiveDisplayPlugin() const;

    float getRenderResolutionScale() const;

    void releaseSceneTexture(uint32_t texture);
    virtual void releaseOverlayTexture(uint32_t texture);
    void setFullscreen(QScreen* screen);
    void unsetFullscreen();

    QOpenGLContext* getPrimaryRenderingContext() override;
    bool makePrimaryRenderingContextCurrent() override;
    void restoreDefaultFramebuffer();

    
signals:
    void activeDisplayPluginChanged();

public slots:
    void resetSensors(bool andReload = false);
    virtual void idle() override;
    void setActiveDisplayPlugin(QString);

protected slots:
    void updateDisplayMode();
    void updateInputModes();

protected:

    virtual void updateOverlayTexture(uint32_t textureId, const glm::uvec2& size) override;
    virtual void cleanupBeforeQuit() override;
    virtual void initializeUI(const QUrl& desktopUrl) override;
    void update(float deltaTime);

    OffscreenGLCanvas* _offscreenContext { nullptr };
    DisplayPluginPointer _displayPlugin;
    InputPluginList _activeInputPlugins;
    bool _activatingDisplayPlugin { false };
    std::shared_ptr<controller::StateController> _applicationStateDevice; // Default ApplicationDevice reflecting the state of different properties of the session
    uvec2 _renderResolution;
    QOpenGLFramebufferObject* _currentFramebuffer { nullptr };
    DisplayPluginPointer _newDisplayPlugin;
    FboCache _fboCache;
    bool _pendingPaint { false };
    QString _fixedPlugin { "Oculus Rift" };

};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<PluginApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
