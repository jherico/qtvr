//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <gl/GLWindow.h>
#include <Platform.h>
#include <plugins/DisplayPlugin.h>
#include <gl/OglplusHelpers.h>
#include <OffscreenUi.h>


Application::Application(int& argc, char** argv) : PluginApplication(QUrl::fromLocalFile("shadertoy/AppDesktop.qml"), argc, argv) {
    getWindow()->setGeometry(100, -980, 1280, 720);
    Q_INIT_RESOURCE(ShadertoyVR);
    _renderer.setup();
    getOffscreenUi()->setRootContextProperty("Renderer", &_renderer);
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
    auto displayPlugin = getActiveDisplayPlugin();
    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();
    auto size = displayPlugin->getRecommendedRenderSize();
    using namespace oglplus;
    if (displayPlugin->isHmd()) {
        for_each_eye([&] (Eye eye){
            Context::Viewport(eye == Eye::Left ? 0 : size.x / 2, 0, size.x / 2, size.y);
            pr.top() = displayPlugin->getProjection(eye, mat4());
            mv.top() = glm::inverse(displayPlugin->getHeadPose(getFrameCount()));
            _renderer.render();
        });
    } else {
        Context::Viewport(size.x, size.y);
        _renderer.render();
    }
}
