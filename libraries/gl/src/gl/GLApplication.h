//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLApplication_h
#define hifi_GLApplication_h

#include <stdint.h>
#include <HifiApplication.h>
#include <shared/RateCounter.h>

class GLWindow;
class QOpenGLContext;
class OffscreenGLCanvas;

class GLApplication : public HifiApplication  {
    Q_OBJECT
    
public:
    enum CustomEventTypes {
        Render = HifiApplication::LastHifiApplicationEvent + 1
    };


    GLApplication(int& argc, char** argv);
    virtual ~GLApplication();

    bool event(QEvent* event);

    GLWindow* getWindow();
    const GLWindow* getWindow() const;

    bool isActive() const;
    uint32_t getFrameCount() const { return _frameCount; }
    float getFps() const { return _frameCounter.rate(); }

protected:
    virtual void initializeGL();
    virtual void prePaintGL();
    virtual void paintGL();
    virtual void resizeGL();
    virtual void submitGL();
    virtual QOpenGLContext* getPrimaryRenderingContext();
    virtual bool makePrimaryRenderingContextCurrent();

    bool _inPaint { false };

private:
    void internalPaintGL();

    GLWindow* _window { nullptr };

    // Frame Rate Measurement
    uint32_t _frameCount { 0 };
    RateCounter<> _frameCounter;
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<GLApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
