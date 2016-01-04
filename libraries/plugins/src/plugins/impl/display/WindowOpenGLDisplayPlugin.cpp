//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WindowOpenGLDisplayPlugin.h"

#include <QtGui/QWindow>

#include <gl/GLWindow.h>
#include "../../PluginApplication.h"

uvec2 WindowOpenGLDisplayPlugin::getSurfacePixels() const {
    uvec2 result;
    if (_window) {
        result = toGlm(_window->geometry().size() * _window->devicePixelRatio());
    }
    return result;
}

uvec2 WindowOpenGLDisplayPlugin::getSurfaceSize() const {
    uvec2 result;
    if (_window) {
        result = toGlm(_window->geometry().size());
    }
    return result;
}

bool WindowOpenGLDisplayPlugin::hasFocus() const {
    return qApp->getWindow()->isActive();
}

void WindowOpenGLDisplayPlugin::activate() {
    _window = qApp->getWindow();
    OpenGLDisplayPlugin::activate();
}

void WindowOpenGLDisplayPlugin::deactivate() {
    OpenGLDisplayPlugin::deactivate();
    _window = nullptr;
}

