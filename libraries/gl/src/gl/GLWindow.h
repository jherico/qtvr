//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GLWindow_h
#define hifi_GLWindow_h

#include <mutex>
#include <QtGui/QWindow>

class QOpenGLContextWrapper;
class QOpenGLDebugLogger;

class GLWindow : public QWindow {
    Q_OBJECT
public:
    GLWindow(QObject* parent = nullptr);
    virtual ~GLWindow();
    void createContext(QOpenGLContext* shareContext = nullptr);
    void createContext(const QSurfaceFormat& format, QOpenGLContext* shareContext = nullptr);
    bool makeCurrent();
    void doneCurrent();
    void swapBuffers();
    QOpenGLContextWrapper* context() const;

signals:
    void aboutToClose();

private:
    friend class CloseEventFilter;
    void emitClosing();
    std::once_flag _reportOnce;
    QOpenGLContextWrapper* _context{ nullptr };
};

#endif
