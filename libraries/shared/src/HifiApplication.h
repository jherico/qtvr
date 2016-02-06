//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_HifiApplication_h
#define hifi_HifiApplication_h

#include <functional>

#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtCore/QElapsedTimer>

#include <QtGui/qevent.h>
#include <QtGui/QImage>
#include <QtGui/QGuiApplication>

class FileLogger;

class HifiApplication : public QGuiApplication  {
    Q_OBJECT

public:
    HifiApplication(int& argc, char** argv);
    virtual ~HifiApplication();

    void postLambdaEvent(std::function<void()> f);
    bool event(QEvent* event);

    FileLogger* getLogger() { return _logger; }

    bool isAboutToQuit() const { return _aboutToQuit; }

    virtual bool canAcceptURL(const QString& url) const { return false; }
    virtual bool acceptURL(const QString& url, bool defaultUpload = false) { return false; }

    enum CustomEventTypes {
        Lambda = QEvent::User + 1,
        LastHifiApplicationEvent = Lambda
    };

signals:
    void beforeAboutToQuit();

public slots:
    void crashApplication();

protected slots:
    virtual void idle();
    virtual void aboutToQuit();
    virtual void activeChanged(Qt::ApplicationState state);
    
protected:
    virtual void cleanupBeforeQuit();
    virtual void update(float deltaTime);

    QElapsedTimer _lastTimeUpdated;
private:
    bool _aboutToQuit { false };
    bool _isForeground { true };
    FileLogger* _logger;
    QTimer _idleTimer;
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<HifiApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
