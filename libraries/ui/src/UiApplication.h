//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UiApplication_h
#define hifi_UiApplication_h

#include <GLMHelpers.h>
#include <gl/GLApplication.h>

class OffscreenUi;

class UiApplication : public GLApplication {
    Q_OBJECT
    
public:
    UiApplication(int& argc, char** argv);
    virtual ~UiApplication();

    OffscreenUi* getOffscreenUi() { return _offscreenUi; }
    const OffscreenUi* getOffscreenUi() const { return _offscreenUi; }

protected slots:
    virtual void updateOverlayTexture(uint32_t textureId);

protected:
    virtual void cleanupBeforeQuit() override;
    virtual void onAction(int action, float value);

    virtual void initializeUI(const QUrl& desktopUrl);
    void resizeUi(const glm::uvec2& newSize);

protected:
    OffscreenUi* _offscreenUi { nullptr };
    bool _reticleClickPressed { false };

    // FIXME hack access to the internal share context for the Chromium helper
    // Normally we'd want to use QWebEngine::initialize(), but we can't because 
    // our primary context is a QGLWidget, which can't easily be initialized to share
    // from a QOpenGLContext.
    //
    // So instead we create a new offscreen context to share with the QGLWidget,
    // and manually set THAT to be the shared context for the Chromium helper
    OffscreenGLCanvas* _chromiumShareContext { nullptr };
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<UiApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
