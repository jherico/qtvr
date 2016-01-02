//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <stdint.h>
#include <QString>
#include <QtCore/QVector>
#include <QtCore/QPair>

#include "Forward.h"

class QScreen;
class QOpenGLContextWrapper;
class QWindow;

class DisplayPlugin;

class PluginContainer {
public:
    static PluginContainer& getInstance();
    PluginContainer();
    virtual ~PluginContainer();
    virtual void addMenu(const QString& menuName) = 0;
    virtual void removeMenu(const QString& menuName) = 0;
    virtual void addMenuItem(PluginType pluginType, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual void removeMenuItem(const QString& menuName, const QString& menuItem) = 0;
    virtual bool isOptionChecked(const QString& name) = 0;
    virtual void clearDisplayPluginItems() = 0;
    virtual void clearInputPluginItems() = 0;
    virtual void setIsOptionChecked(const QString& path, bool checked) = 0;
    virtual void setFullscreen(const QScreen* targetScreen, bool hideMenu = false) = 0;
    virtual void unsetFullscreen(const QScreen* avoidScreen = nullptr) = 0;
    virtual void showDisplayPluginsTools() = 0;
    virtual void requestReset() = 0;
    virtual bool makeRenderingContextCurrent() = 0;
    virtual void releaseSceneTexture(uint32_t texture) = 0;
    virtual void releaseOverlayTexture(uint32_t texture) = 0;
    virtual QWindow* getPrimaryWindow() = 0;
    virtual QOpenGLContextWrapper* getPrimaryContext() = 0;
    virtual bool isForeground() = 0;
    virtual const DisplayPlugin* getActiveDisplayPlugin() const = 0;

    QVector<QPair<QString, QString>>& currentDisplayActions() {
        return _currentDisplayPluginActions;
    }

    QVector<QPair<QString, QString>>& currentInputActions() {
        return _currentInputPluginActions;
    }

protected:
    QVector<QPair<QString, QString>> _currentDisplayPluginActions;
    QVector<QPair<QString, QString>> _currentInputPluginActions;

};
