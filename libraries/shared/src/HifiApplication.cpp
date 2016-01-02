//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HifiApplication.h"

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include "SharedUtil.h"
#include "FileLogger.h"
#include "Menu.h"

#include "SettingHandle.h"
#include "LogHandler.h"
#include "PathUtils.h"

Q_LOGGING_CATEGORY(interfaceapp, "hifi.interface")
Q_LOGGING_CATEGORY(interfaceapp_timing, "hifi.interface.timing")

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

using namespace std;

#ifndef __APPLE__
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/script.js");
#endif

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        qApp->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

HifiApplication::HifiApplication(int& argc, char** argv) : QGuiApplication(argc, argv) {
    // Set build version
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    Setting::preInit();
    Setting::init();

    // Set dependencies
    DependencyManager::set<PathUtils>();

    thread()->setObjectName("Main Thread");

    // to work around the Qt constant wireless scanning, set the env for polling interval very high
    const QByteArray EXTREME_BEARER_POLL_TIMEOUT = QString::number(INT_MAX).toLocal8Bit();
    qputenv("QT_BEARER_POLL_TIMEOUT", EXTREME_BEARER_POLL_TIMEOUT);

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);

    qDebug() << "[VERSION] Build sequence:" << qPrintable(applicationVersion());

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
    connect(this, &HifiApplication::applicationStateChanged, this, &HifiApplication::activeChanged);

    _idleTimer.setInterval(0);
    connect(&_idleTimer, &QTimer::timeout, [this] { idle(); });
    _idleTimer.start();
}

void HifiApplication::aboutToQuit() {
    emit beforeAboutToQuit();
    _aboutToQuit = true;
    cleanupBeforeQuit();
}

void HifiApplication::cleanupBeforeQuit() {
}

HifiApplication::~HifiApplication() {
    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

bool HifiApplication::event(QEvent* event) {
    return QGuiApplication::event(event);
}

void HifiApplication::idle() {
    if (_aboutToQuit) {
        return; // bail early, nothing to do here.
    }

    uint64_t now = usecTimestampNow();
    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    // This attempts to lock the simulation at 60 updates per second, regardless of framerate
    float timeSinceLastUpdateUs = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC;
    float secondsSinceLastUpdate = timeSinceLastUpdateUs / USECS_PER_SECOND;
    _lastTimeUpdated.start();

    static const float BIGGEST_DELTA_TIME_SECS = 0.25f;
    update(glm::clamp(secondsSinceLastUpdate, 0.0f, BIGGEST_DELTA_TIME_SECS));
}


void HifiApplication::update(float deltaTime) {
}


void HifiApplication::activeChanged(Qt::ApplicationState state) {
    switch (state) {
        case Qt::ApplicationActive:
            _isForeground = true;
            break;

        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
        default:
            _isForeground = false;
            break;
    }
}

void HifiApplication::postLambdaEvent(std::function<void()> f) {
}

void HifiApplication::crashApplication() {
    qCDebug(interfaceapp) << "Intentionally crashed Interface";
    int* value = nullptr;
    *value = 1;
}

