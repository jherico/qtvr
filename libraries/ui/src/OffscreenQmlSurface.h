//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenQmlSurface_h
#define hifi_OffscreenQmlSurface_h

#include <mutex>
#include <atomic>
#include <functional>

#include <QTimer>
#include <QUrl>

#include <GLMHelpers.h>
#include <ThreadHelpers.h>

class QWindow;
class QMyQuickRenderControl;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;

class OffscreenQmlRenderThread;

class OffscreenQmlSurface : public QObject {
    Q_OBJECT

public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPoint(const QPointF&)>;

    virtual void create(QOpenGLContext* context);
    void resize(const QSize& size);
    QSize size() const;

    Q_INVOKABLE void executeOnUiThread(std::function<void()> function, bool blocking = false);
    Q_INVOKABLE QVariant returnFromUiThread(std::function<QVariant()> function);

    Q_INVOKABLE QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    Q_INVOKABLE QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

    void setMaxFps(uint8_t maxFps) { _maxFps = maxFps; }
    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }

    void pause();
    void resume();
    bool isPaused() const;

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getRootContext();

    QPointF mapToVirtualScreen(const QPointF& originalPoint, QWindow* source) const;
    bool eventFilter(QObject* originalDestination, QEvent* event) override;

    //uint32_t getTexture() const;
    void lockTexture(uint32_t texture);
    void releaseTexture(uint32_t texture);

private slots:
    void requestUpdate();
    void requestRender();
    void onAboutToQuit();
    void updateQuick();

protected:
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;

signals:
    //void textureUpdated(unsigned int texture, const glm::uvec2& size);
    void textureUpdated(unsigned int texture);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QWindow* sourceObject) const;
    void releaseTextureInternal(uint32_t texture);

private:
    friend class OffscreenQmlRenderThread;
    OffscreenQmlRenderThread* _renderer{ nullptr };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    //uint32_t _currentTexture{ 0 };
    bool _render{ false };
    bool _polish{ true };
    bool _paused{ true };
    uint8_t _maxFps{ 60 };
    MouseTranslator _mouseTranslator { [](const QPointF& p) { return p.toPoint();  } };
    QWindow* _proxyWindow { nullptr };
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    std::map<uint32_t, uint32_t> _textureLocks;
    mutable Mutex _mutex;
};

#endif
