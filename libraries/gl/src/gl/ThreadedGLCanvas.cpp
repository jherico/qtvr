//
//  ThreadedGLCanvas.cpp
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "ThreadedGLCanvas.h"

#include <QtCore/QEvent>

#include "GLHelpers.h"
#include "GLApplication.h"

ThreadedGLCanvas::ThreadedGLCanvas(QOpenGLContext* shareContext) {
    create(shareContext);
    qApp->makePrimaryRenderingContextCurrent();
    getContextObject()->moveToThread(&_thread);
    moveToThread(&_thread);
    _thread.setObjectName("QML Thread");
    _thread.start();
}

ThreadedGLCanvas::~ThreadedGLCanvas() {
    doneCurrent();
}

static const QEvent::Type EXECUTE = QEvent::Type(QEvent::User + 1);

class QBackgroundGLEvent : public QEvent {
public:
    QBackgroundGLEvent(std::function<void()> f) 
        : QEvent(EXECUTE), _f(f) {}
    void run() { _f(); }

private:
    const std::function<void()> _f;
    const QFuture<void>* _future { nullptr };
};

void ThreadedGLCanvas::execute(std::function<void()> f) {
    QCoreApplication::postEvent(this, new QBackgroundGLEvent(f));
}

bool ThreadedGLCanvas::event(QEvent *e) {
    switch (e->type()) {
        case EXECUTE:
            makeCurrent();
            ((QBackgroundGLEvent*)e)->run();
            doneCurrent();
            return true;
    }
    return OffscreenGLCanvas::event(e);
}
