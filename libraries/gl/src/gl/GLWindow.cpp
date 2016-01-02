//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLWindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include "GLHelpers.h"
#include "QOpenGLContextWrapper.h"

void GLWindow::createContext(QOpenGLContext* shareContext) {
    createContext(getDefaultOpenGLSurfaceFormat(), shareContext);
}

void GLWindow::createContext(const QSurfaceFormat& format, QOpenGLContext* shareContext) {
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(format);
    _context = new QOpenGLContextWrapper();
    _context->setFormat(format);
    if (shareContext) {
        _context->setShareContext(shareContext);
    }
    _context->create();
}

GLWindow::~GLWindow() {
    if (_context) {
        _context->doneCurrent();
        _context->deleteLater();
        _context = nullptr;
    }
}

bool GLWindow::makeCurrent() {
    bool makeCurrentResult = _context->makeCurrent(this);
    Q_ASSERT(makeCurrentResult);
    
    std::call_once(_reportOnce, []{
        qDebug() << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
        qDebug() << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        qDebug() << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));
        qDebug() << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));
    });
    
    Q_ASSERT(_context->isCurrentContext());
    
    return makeCurrentResult;
}

void GLWindow::doneCurrent() {
    _context->doneCurrent();
}

void GLWindow::swapBuffers() {
    _context->swapBuffers(this);
}

QOpenGLContextWrapper* GLWindow::context() const {
    return _context;
}


