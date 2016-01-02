//
//  Created by Bradley Austin Davis on 2015-05-13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenQmlSurface.h"

#include <gl/Config.h>

#include <QtQml/QtQml>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickRenderControl>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

#include <shared/NsightHelpers.h>

#include <gl/OglplusHelpers.h>
#include <gl/OffscreenGLCanvas.h>
#include <gl/GLEscrow.h>

#include <PerfStat.h>
#include <DependencyManager.h>
#include <NumericalConstants.h>



// Time between receiving a request to render the offscreen UI actually triggering
// the render.  Could possibly be increased depending on the framerate we expect to
// achieve.
// This has the effect of capping the framerate at 200
static const int MIN_TIMER_MS = 5;

class QMyQuickRenderControl : public QQuickRenderControl {
protected:
    QWindow* renderWindow(QPoint* offset) Q_DECL_OVERRIDE{
        if (nullptr == _renderWindow) {
            return QQuickRenderControl::renderWindow(offset);
        }
        if (nullptr != offset) {
            offset->rx() = offset->ry() = 0;
        }
        return _renderWindow;
    }

private:
    QWindow* _renderWindow{ nullptr };
    friend class OffscreenQmlRenderer;
    friend class OffscreenQmlSurface;
};


Q_DECLARE_LOGGING_CATEGORY(offscreenFocus)
Q_LOGGING_CATEGORY(offscreenFocus, "hifi.offscreen.focus")

static const QEvent::Type INIT = QEvent::Type(QEvent::User + 1);
static const QEvent::Type RENDER = QEvent::Type(QEvent::User + 2);
static const QEvent::Type RESIZE = QEvent::Type(QEvent::User + 3);
static const QEvent::Type STOP = QEvent::Type(QEvent::User + 4);

class OffscreenQmlRenderer : public OffscreenGLCanvas {
    friend class OffscreenQmlSurface;
public:

    OffscreenQmlRenderer(OffscreenQmlSurface* surface, QOpenGLContext* shareContext) : _surface(surface) {
        OffscreenGLCanvas::create(shareContext);

        _renderControl = new QMyQuickRenderControl();

        // Create a QQuickWindow that is associated with out render control. Note that this
        // window never gets created or shown, meaning that it will never get an underlying
        // native (platform) window.
        QQuickWindow::setDefaultAlphaBuffer(true);
        // Weirdness...  QQuickWindow NEEDS to be created on the rendering thread, or it will refuse to render
        // because it retains an internal 'context' object that retains the thread it was created on, 
        // regardless of whether you later move it to another thread.
        _quickWindow = new QQuickWindow(_renderControl);
        _quickWindow->setColor(QColor(255, 255, 255, 0));
        _quickWindow->setFlags(_quickWindow->flags() | static_cast<Qt::WindowFlags>(Qt::WA_TranslucentBackground));

        // Qt 5.5
        _renderControl->prepareThread(&_thread);
        getContextObject()->moveToThread(&_thread);
        moveToThread(&_thread);
        _thread.setObjectName("QML Thread");
        _thread.start();
        post(INIT);
    }

    bool event(QEvent *e) {
        switch (int(e->type())) {
        case INIT:
            {
                QMutexLocker lock(&_mutex);
                init();
            }
            return true;
        case RENDER:
            {
                QMutexLocker lock(&_mutex);
                render(&lock);
            }
            return true;
        case RESIZE:
            {
                QMutexLocker lock(&_mutex);
                resize();
            }
            return true;
        case STOP:
            {
                QMutexLocker lock(&_mutex);
                cleanup();
            }
            return true;
        default:
            return QObject::event(e);
        }
    }

    void post(const QEvent::Type& type) {
        QCoreApplication::postEvent(this, new QEvent(type));
    }


private:

    void setupFbo() {
        using namespace oglplus;
        _textures.setSize(_size);
        _depthStencil.reset(new Renderbuffer());
        Context::Bound(Renderbuffer::Target::Renderbuffer, *_depthStencil)
            .Storage(
            PixelDataInternalFormat::DepthComponent,
            _size.x, _size.y);

        _fbo.reset(new Framebuffer());
        _fbo->Bind(Framebuffer::Target::Draw);
        _fbo->AttachRenderbuffer(Framebuffer::Target::Draw, 
            FramebufferAttachment::Depth, *_depthStencil);
        DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
    }



    void init() {
        connect(_renderControl, &QQuickRenderControl::renderRequested, _surface, &OffscreenQmlSurface::requestRender);
        connect(_renderControl, &QQuickRenderControl::sceneChanged, _surface, &OffscreenQmlSurface::requestUpdate);

        if (!makeCurrent()) {
            qWarning("Failed to make context current on render thread");
            return;
        }
        _renderControl->initialize(_context);
        setupFbo();
        _escrow.setRecycler([this](GLuint texture){
            _textures.recycleTexture(texture);
        });
        doneCurrent();
    }

    void cleanup() {
        if (!makeCurrent()) {
            qFatal("Failed to make context current on render thread");
            return;
        }
        _renderControl->invalidate();

        _fbo.reset();
        _depthStencil.reset();
        _textures.clear();

        doneCurrent();

        getContextObject()->moveToThread(QCoreApplication::instance()->thread());
        _cond.wakeOne();
    }

    void resize() {
        // Update our members
        if (_quickWindow) {
            _quickWindow->setGeometry(QRect(QPoint(), _newSize));
            _quickWindow->contentItem()->setSize(_newSize);
        }

        // Qt bug in 5.4 forces this check of pixel ratio,
        // even though we're rendering offscreen.
        qreal pixelRatio = 1.0;
        if (_renderControl && _renderControl->_renderWindow) {
            pixelRatio = _renderControl->_renderWindow->devicePixelRatio();
        } 

        uvec2 newOffscreenSize = toGlm(_newSize * pixelRatio);
        _textures.setSize(newOffscreenSize);
        if (newOffscreenSize == _size) {
            return;
        }
        _size = newOffscreenSize;

        // Clear out any fbos with the old size
        if (!makeCurrent()) {
            qWarning("Failed to make context current on render thread");
            return;
        }

        qDebug() << "Offscreen UI resizing to " << _newSize.width() << "x" << _newSize.height() << " with pixel ratio " << pixelRatio;
        setupFbo();
        doneCurrent();
    }

    void render(QMutexLocker *lock) {
        if (_surface->_paused) {
            return;
        }

        if (!makeCurrent()) {
            qWarning("Failed to make context current on render thread");
            return;
        }

        _renderControl->sync();
        _cond.wakeOne();
        lock->unlock();

        using namespace oglplus;

        _quickWindow->setRenderTarget(GetName(*_fbo), QSize(_size.x, _size.y));

        {
            PROFILE_RANGE("qml_render")
            TexturePtr texture = _textures.getNextTexture();
            _fbo->Bind(Framebuffer::Target::Draw);
            _fbo->AttachTexture(Framebuffer::Target::Draw, FramebufferAttachment::Color, *texture, 0);
            _fbo->Complete(Framebuffer::Target::Draw);
            {
                _renderControl->render();
                // FIXME The web browsers seem to be leaving GL in an error state.
                // Need a debug context with sync logging to figure out why.
                // for now just clear the errors
                glGetError();
            }
            // FIXME probably unecessary
            DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
            _quickWindow->resetOpenGLState();
            _escrow.submit(GetName(*texture));
        }
        _lastRenderTime = usecTimestampNow();
    }

    void aboutToQuit() {
        QMutexLocker lock(&_quitMutex);
        _quit = true;
    }

    static const uint64_t MAX_SHUTDOWN_WAIT_SECS = 5;
    void stop() {
        if (_thread.isRunning()) {
            qDebug() << "Stopping QML render thread " << _thread.currentThreadId();
            {
                QMutexLocker lock(&_mutex);
                post(STOP);
            }
            auto start = usecTimestampNow();
            auto now = usecTimestampNow();
            bool shutdownClean = false;
            while (now - start < (MAX_SHUTDOWN_WAIT_SECS * USECS_PER_SECOND)) {
                QMutexLocker lock(&_mutex);
                if (_cond.wait(&_mutex, MSECS_PER_SECOND)) {
                    shutdownClean = true;
                    break;
                }
                now = usecTimestampNow();
            }

            if (!shutdownClean) {
                qWarning() << "Failed to shut down the QML render thread";
            }

        } else {
            qDebug() << "QML render thread already completed";
        }
    }

    bool allowNewFrame(uint8_t fps) {
        auto minRenderInterval = USECS_PER_SECOND / fps;
        auto lastInterval = usecTimestampNow() - _lastRenderTime;
        return (lastInterval > minRenderInterval);
    }

    OffscreenQmlSurface* _surface{ nullptr };
    QQuickWindow* _quickWindow{ nullptr };
    QMyQuickRenderControl* _renderControl{ nullptr };

    QThread _thread;
    QMutex _mutex;
    QWaitCondition _cond;
    QMutex _quitMutex;

    QSize _newSize;
    bool _quit;
    FramebufferPtr _fbo;
    RenderbufferPtr _depthStencil;
    uvec2 _size{ 1920, 1080 };
    uint64_t _lastRenderTime{ 0 };
    TextureRecycler _textures;
    GLTextureEscrow _escrow;
};

OffscreenQmlSurface::OffscreenQmlSurface() {
}

OffscreenQmlSurface::~OffscreenQmlSurface() {
    _renderer->stop();

    delete _renderer;
    delete _qmlComponent;
    delete _qmlEngine;
}

void OffscreenQmlSurface::create(QOpenGLContext* shareContext) {
    _renderer = new OffscreenQmlRenderer(this, shareContext);

    // Create a QML engine.
    _qmlEngine = new QQmlEngine;
    if (!_qmlEngine->incubationController()) {
        _qmlEngine->setIncubationController(_renderer->_quickWindow->incubationController());
    }

    // When Quick says there is a need to render, we will not render immediately. Instead,
    // a timer with a small interval is used to get better performance.
    _updateTimer.setInterval(MIN_TIMER_MS);
    connect(&_updateTimer, &QTimer::timeout, this, &OffscreenQmlSurface::updateQuick);
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [this]{
        disconnect(&_updateTimer, &QTimer::timeout, this, &OffscreenQmlSurface::updateQuick);
    });
    _updateTimer.start();

    _qmlComponent = new QQmlComponent(_qmlEngine);
}

void OffscreenQmlSurface::resize(const QSize& newSize) {

    if (!_renderer || !_renderer->_quickWindow) {
        return;
    }


    QSize currentSize = _renderer->_quickWindow->geometry().size();
    if (newSize == currentSize) {
        return;
    }

    _qmlEngine->rootContext()->setContextProperty("surfaceSize", newSize);

    if (_rootItem) {
        _rootItem->setSize(newSize);
    }

    {
        QMutexLocker _locker(&(_renderer->_mutex));
        _renderer->_newSize = newSize;
    }

    _renderer->post(RESIZE);
}

QQuickItem* OffscreenQmlSurface::getRootItem() {
    return _rootItem;
}

void OffscreenQmlSurface::setBaseUrl(const QUrl& baseUrl) {
    _qmlEngine->setBaseUrl(baseUrl);
}

QObject* OffscreenQmlSurface::load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f) {
    _qmlComponent->loadUrl(qmlSource);
    if (_qmlComponent->isLoading()) {
        connect(_qmlComponent, &QQmlComponent::statusChanged, this, 
            [this, f](QQmlComponent::Status){
                finishQmlLoad(f);
            });
        return nullptr;
    }
    
    return finishQmlLoad(f);
}

void OffscreenQmlSurface::requestUpdate() {
    _polish = true;
    _render = true;
}

void OffscreenQmlSurface::requestRender() {
    _render = true;
}

QObject* OffscreenQmlSurface::finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f) {
    disconnect(_qmlComponent, &QQmlComponent::statusChanged, this, 0);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList) {
            qWarning() << error.url() << error.line() << error;
        }
        return nullptr;
    }

    QQmlContext* newContext = new QQmlContext(_qmlEngine, qApp);
    QObject* newObject = _qmlComponent->beginCreate(newContext);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList)
            qWarning() << error.url() << error.line() << error;
        if (!_rootItem) {
            qFatal("Unable to finish loading QML root");
        }
        return nullptr;
    }

    f(newContext, newObject);
    _qmlComponent->completeCreate();


    // All quick items should be focusable
    QQuickItem* newItem = qobject_cast<QQuickItem*>(newObject);
    if (newItem) {
        // Make sure we make items focusable (critical for 
        // supporting keyboard shortcuts)
        newItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    }

    // If we already have a root, just set a couple of flags and the ancestry
    if (_rootItem) {
        // Allow child windows to be destroyed from JS
        QQmlEngine::setObjectOwnership(newObject, QQmlEngine::JavaScriptOwnership);
        newObject->setParent(_rootItem);
        if (newItem) {
            newItem->setParentItem(_rootItem);
        }
        return newObject;
    }

    if (!newItem) {
        qFatal("Could not load object as root item");
        return nullptr;
    }
    // The root item is ready. Associate it with the window.
    _rootItem = newItem;
    _rootItem->setParentItem(_renderer->_quickWindow->contentItem());
    _rootItem->setSize(_renderer->_quickWindow->renderTargetSize());
    return _rootItem;
}


void OffscreenQmlSurface::updateQuick() {
    if (!_renderer || !_renderer->allowNewFrame(_maxFps)) {
        return;
    }

    if (_polish) {
        _renderer->_renderControl->polishItems();
        _polish = false;
    }

    if (_render) {
        QMutexLocker lock(&(_renderer->_mutex));
        _renderer->post(RENDER);
        _renderer->_cond.wait(&(_renderer->_mutex));
        _render = false;
    }

    GLuint newTexture = _renderer->_escrow.fetch();
    if (newTexture) {
        {
            Lock lock(_mutex);
            if (_currentTexture) {
                releaseTextureInternal(_currentTexture);
                _currentTexture = 0;
            }
            _currentTexture = newTexture;
            _textureLocks[_currentTexture] = 0;
        }
        emit textureUpdated(_currentTexture);
    }
}

uint32_t OffscreenQmlSurface::getTexture() const {
    Lock lock(_mutex);
    return _currentTexture;
}

void OffscreenQmlSurface::lockTexture(uint32_t texture) {
    Lock lock(_mutex);
    auto& useCount = _textureLocks[texture];
    ++useCount;
}

void OffscreenQmlSurface::releaseTexture(uint32_t texture) {
    Lock lock(_mutex);
    releaseTextureInternal(texture);
}

void OffscreenQmlSurface::releaseTextureInternal(uint32_t texture) {
    auto& useCount = _textureLocks[texture];
    --useCount;
    if (0 == useCount) {
        _renderer->_escrow.release(_currentTexture);
        _textureLocks.erase(texture);
    }
}

QPointF OffscreenQmlSurface::mapWindowToUi(const QPointF& sourcePosition, QWindow* source) const {
    vec2 sourceSize;
    sourceSize = toGlm(source->size());
    vec2 offscreenPosition = toGlm(sourcePosition);
    offscreenPosition /= sourceSize;
    offscreenPosition *= vec2(toGlm(_renderer->_quickWindow->size()));
    return QPointF(offscreenPosition.x, offscreenPosition.y);
}

QPointF OffscreenQmlSurface::mapToVirtualScreen(const QPointF& originalPoint, QWindow* source) const {
    return _mouseTranslator(originalPoint);
}


///////////////////////////////////////////////////////
//
// Event handling customization
//

bool OffscreenQmlSurface::eventFilter(QObject* originalDestination, QEvent* event) {
    if (_renderer->_quickWindow == originalDestination) {
        return false;
    }
    // Only intercept events while we're in an active state
    if (_paused) {
        return false;
    }

    QWindow* window = dynamic_cast<QWindow*>(originalDestination);

#ifdef DEBUG
    // Don't intercept our own events, or we enter an infinite recursion
    QObject* recurseTest = originalDestination;
    while (recurseTest) {
        Q_ASSERT(recurseTest != _rootItem && recurseTest != _renderer->_quickWindow);
        recurseTest = recurseTest->parent();
    }
#endif

   
    switch (event->type()) {
        case QEvent::Resize: {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
            if (window) {
                this->resize(resizeEvent->size());
            }
            break;
        }

        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            event->ignore();
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, event)) {
                return event->isAccepted();
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(wheelEvent->pos(), window);
            QWheelEvent mappedEvent(
                    transformedPos,
                    wheelEvent->delta(),  wheelEvent->buttons(),
                    wheelEvent->modifiers(), wheelEvent->orientation());
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        // Fall through
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(mouseEvent->localPos(), window);
            QMouseEvent mappedEvent(mouseEvent->type(),
                    transformedPos,
                    mouseEvent->screenPos(), mouseEvent->button(),
                    mouseEvent->buttons(), mouseEvent->modifiers());
            if (event->type() == QEvent::MouseMove) {
                _qmlEngine->rootContext()->setContextProperty("lastMousePosition", transformedPos);
            }
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_renderer->_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        default:
            break;
    }

    return false;
}

void OffscreenQmlSurface::pause() {
    _paused = true;
}

void OffscreenQmlSurface::resume() {
    _paused = false;
    requestRender();
}

bool OffscreenQmlSurface::isPaused() const {
    return _paused;
}

void OffscreenQmlSurface::setProxyWindow(QWindow* window) {
    _renderer->_renderControl->_renderWindow = window;
}

QObject* OffscreenQmlSurface::getEventHandler() {
    return getWindow();
}

QQuickWindow* OffscreenQmlSurface::getWindow() {
    return _renderer->_quickWindow;
}

QSize OffscreenQmlSurface::size() const {
    return _renderer->_quickWindow->geometry().size();
}

QQmlContext* OffscreenQmlSurface::getRootContext() {
    return _qmlEngine->rootContext();
}

