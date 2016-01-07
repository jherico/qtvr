//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <gl/GLWindow.h>
#include <Platform.h>


Application::Application(int& argc, char** argv) : UiApplication(argc, argv) {
    getWindow()->setGeometry(100, -980, 800, 600);
    Q_INIT_RESOURCE(ShadertoyVR);
    _renderer.setup();
}

void Application::cleanupBeforeQuit() {
    UiApplication::cleanupBeforeQuit();
}

void Application::aboutToQuit() {
    UiApplication::aboutToQuit();
    getWindow()->makeCurrent();
    Platform::runShutdownHooks();
    getWindow()->doneCurrent();
}

void Application::paintGL() {
    _renderer.render();
}
