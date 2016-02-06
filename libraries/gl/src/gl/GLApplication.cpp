//
//  Created by Bradley Austin Davis on 2015/01/03
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLApplication.h"

#include "Config.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include <NumericalConstants.h>
#include <Finally.h>
#include <shared/NsightHelpers.h>
#include <PerfStat.h>

#include "QOpenGLContextWrapper.h"
#include "GLWindow.h"


Q_LOGGING_CATEGORY(glapplication, "hifi.gl.applicaiton")

// ON WIndows PC, NVidia Optimus laptop, we want to enable NVIDIA GPU
// FIXME seems to be broken.
#if defined(Q_OS_WIN)
extern "C" {
 _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

using namespace std;

GLApplication::GLApplication(int& argc, char** argv) 
    : HifiApplication(argc, argv) {
    _window = new GLWindow();
    _window->setCursor(Qt::BlankCursor);
    _window->createContext();
    _window->setVisible(true);
    _window->setTitle("Test");

    // enable mouse tracking; otherwise, we only get drag events
    _window->setMouseGrabEnabled(true);
    _window->makeCurrent();

    glewExperimental = true;
    GLenum err = glewInit();
    glGetError();

    qCDebug(glapplication) << "Created Display Window.";

    initializeGL();
}

GLApplication::~GLApplication() {
}

void GLApplication::initializeGL() {
}

void GLApplication::internalPaintGL() {
    // paintGL uses a queued connection, so we can get messages from the queue even after we've quit 
    // and the plugins have shutdown
    if (isAboutToQuit()) {
        return;
    }

    // Some plugins process message events, potentially leading to
    // re-entering a paint event.  don't allow further processing if this
    // happens
    if (_inPaint) {
        return;
    }
    _inPaint = true;
    Finally clearFlagLambda([this] { _inPaint = false; });

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

    _frameCount++;

    PROFILE_RANGE(__FUNCTION__);
    resizeGL();
    prePaintGL();
    paintGL();
    submitGL();
}

void GLApplication::prePaintGL() {
    _window->makeCurrent();
}

void GLApplication::paintGL() {
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLApplication::submitGL() {
    _window->swapBuffers();
}

void GLApplication::resizeGL() {
}

bool GLApplication::event(QEvent* event) {
    if ((int)event->type() == (int)Render) {
        internalPaintGL();
    }

    return HifiApplication::event(event);
}

GLWindow* GLApplication::getWindow() {
    return _window;
}

QOpenGLContext* GLApplication::getPrimaryRenderingContext() {
    return _window->context()->getContext();
}

bool GLApplication::makePrimaryRenderingContextCurrent() {
    return _window->makeCurrent();
}

bool GLApplication::isActive() const {
    return _window && _window->isActive();
}