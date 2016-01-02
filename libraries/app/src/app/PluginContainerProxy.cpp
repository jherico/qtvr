#include "PluginContainerProxy.h"

#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <DependencyManager.h>

#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <plugins/DisplayPlugin.h>

#include <gpu/FramebufferCache.h>

#include <gl/GLWindow.h>
#include <gl/OffscreenGLCanvas.h>
#include <OffscreenUi.h>

#include "RenderableApplication.h"

PluginContainerProxy::PluginContainerProxy() {
}

PluginContainerProxy::~PluginContainerProxy() {
}

bool PluginContainerProxy::isForeground() {
    return true; // qApp->isForeground() && !qApp->getWindow()->isMinimized();
}

void PluginContainerProxy::addMenu(const QString& menuName) {
}

void PluginContainerProxy::removeMenu(const QString& menuName) {
}

void PluginContainerProxy::addMenuItem(PluginType type, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable, bool checked, const QString& groupName) {

}

void PluginContainerProxy::removeMenuItem(const QString& menuName, const QString& menuItem) {
}

bool PluginContainerProxy::isOptionChecked(const QString& name) {
    return false;
}

void PluginContainerProxy::setIsOptionChecked(const QString& path, bool checked) {
}

// FIXME there is a bug in the fullscreen setting, where leaving
// fullscreen does not restore the window frame, making it difficult
// or impossible to move or size the window.
// Additionally, setting fullscreen isn't hiding the menu on windows
// make it useless for stereoscopic modes.
void PluginContainerProxy::setFullscreen(const QScreen* target, bool hideMenu) {
    auto _window = qApp->getWindow();
    if (_window->visibility() != QWindow::FullScreen) {
        _savedGeometry = _window->geometry();
    }
    if (nullptr == target) {
        // FIXME target the screen where the window currently is
        target = qApp->primaryScreen();
    }

    _window->setGeometry(target->availableGeometry());
    _window->setScreen((QScreen*)target);
    _window->showFullScreen();
}

void PluginContainerProxy::unsetFullscreen(const QScreen* avoid) {
    auto _window = qApp->getWindow();
    _window->showNormal();

    QRect targetGeometry = _savedGeometry;
    if (avoid != nullptr) {
        QRect avoidGeometry = avoid->geometry();
        if (avoidGeometry.contains(targetGeometry.topLeft())) {
            QScreen* newTarget = qApp->primaryScreen();
            if (newTarget == avoid) {
                foreach(auto screen, qApp->screens()) {
                    if (screen != avoid) {
                        newTarget = screen;
                        break;
                    }
                }
            }
            targetGeometry = newTarget->availableGeometry();
        }
    }
#ifdef Q_OS_MAC
    QTimer* timer = new QTimer();
    timer->singleShot(2000, [=] {
        _window->setGeometry(targetGeometry);
        timer->deleteLater();
    });
#else
    _window->setGeometry(targetGeometry);
#endif
}

void PluginContainerProxy::requestReset() {
    // We could signal qApp to sequence this, but it turns out that requestReset is only used from within the main thread anyway.
    qApp->resetSensors(true);
}

void PluginContainerProxy::showDisplayPluginsTools() {
}

QWindow* PluginContainerProxy::getPrimaryWindow() {
    return qApp->getWindow();
}

QOpenGLContextWrapper* PluginContainerProxy::getPrimaryContext() {
    return qApp->getWindow()->context();
}

const DisplayPlugin* PluginContainerProxy::getActiveDisplayPlugin() const {
    return qApp->getActiveDisplayPlugin();
}

bool PluginContainerProxy::makeRenderingContextCurrent() {
    return qApp->_offscreenContext->makeCurrent();
}

void PluginContainerProxy::releaseSceneTexture(uint32_t texture) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    auto& framebufferMap = qApp->_lockedFramebufferMap;
    Q_ASSERT(framebufferMap.contains(texture));
    auto framebufferPointer = framebufferMap[texture];
    framebufferMap.remove(texture);
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    framebufferCache->releaseFramebuffer(framebufferPointer);
}

void PluginContainerProxy::releaseOverlayTexture(uint32_t texture) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->releaseTexture(texture);
}

void PluginContainerProxy::clearDisplayPluginItems() {
}

void PluginContainerProxy::clearInputPluginItems() {
}


