//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

Application::Application(int& argc, char** argv) : HifiApplication(argc, argv) {
    QCoreApplication::setApplicationName("VulkanTest");
    QCoreApplication::setOrganizationName("Saint Andreas");
    QCoreApplication::setOrganizationDomain("saintandreas.org");

    _window = new VKWindow();
    _window->show();
}

void Application::cleanupBeforeQuit() {
    HifiApplication::cleanupBeforeQuit();
}

void Application::aboutToQuit() {
    delete _window;
}
