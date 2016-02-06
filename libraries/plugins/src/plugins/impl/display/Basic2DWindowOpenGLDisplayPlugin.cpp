//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Basic2DWindowOpenGLDisplayPlugin.h"

#include <mutex>

#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>

#include "../../PluginApplication.h"

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("2D Display");

const QString& Basic2DWindowOpenGLDisplayPlugin::getName() const {
    return NAME;
}

void Basic2DWindowOpenGLDisplayPlugin::internalPresent() {
    if (_wantVsync != isVsyncEnabled()) {
        enableVsync(_wantVsync);
    }
    WindowOpenGLDisplayPlugin::internalPresent();
}

bool Basic2DWindowOpenGLDisplayPlugin::isThrottled() const {
    return !qApp->isActive();
}

// FIXME target the screen the window is currently on
QScreen* Basic2DWindowOpenGLDisplayPlugin::getFullscreenTarget() {
    return qApp->primaryScreen();
}
