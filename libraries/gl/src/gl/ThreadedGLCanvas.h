//
//  OffscreenGLCanvas.h
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ThreadedGLCanvas_h
#define hifi_ThreadedGLCanvas_h

#include <QtCore/QThread>
#include <QtCore/QFuture>

#include "OffscreenGLCanvas.h"

class ThreadedGLCanvas : public OffscreenGLCanvas {
public:
    ThreadedGLCanvas(QOpenGLContext* shareContext);
    ~ThreadedGLCanvas();
    

    void execute(std::function<void()> f);
protected:
    bool event(QEvent *e) override;

    QThread _thread;
};

#endif // hifi_OffscreenGLCanvas_h
