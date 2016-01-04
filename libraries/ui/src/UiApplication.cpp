//
//  Application.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UiApplication.h"

#include <QtCore/QLoggingCategory>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <PathUtils.h>

#include <gl/Config.h>
#include <QtQuick/QQuickWindow>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/OffscreenGLCanvas.h>
#include <gl/GLWindow.h>

#include <plugins/DisplayPlugin.h>

#include <controllers/UserInputMapper.h>
#include <controllers/StateController.h>


#include "MessageDialog.h"
#include "VrMenu.h"
#include "OffscreenUi.h"

Q_DECLARE_LOGGING_CATEGORY(interfaceapp)
Q_DECLARE_LOGGING_CATEGORY(interfaceapp_timing)

// FIXME hack access to the internal share context for the Chromium helper
// Normally we'd want to use QWebEngine::initialize(), but we can't because 
// our primary context is a QGLWidget, which can't easily be initialized to share
// from a QOpenGLContext.
//
// So instead we create a new offscreen context to share with the QGLWidget,
// and manually set THAT to be the shared context for the Chromium helper
OffscreenGLCanvas* _chromiumShareContext { nullptr };
Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);

UiApplication::UiApplication(int& argc, char** argv)
    : PluginApplication(argc, argv) {

    _chromiumShareContext = new OffscreenGLCanvas();
    _chromiumShareContext->create(getWindow()->context()->getContext());
    _chromiumShareContext->makeCurrent();
    qt_gl_set_global_share_context(_chromiumShareContext->getContext());

    initializeUI();
    makeRenderingContextCurrent();

    // Setup the userInputMapper with the actions
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, [this](int action, float state) {
        using namespace controller;
        if (_offscreenUi->navigationFocused()) {
            auto actionEnum = static_cast<Action>(action);
            int key = Qt::Key_unknown;
            static int lastKey = Qt::Key_unknown;
            bool navAxis = false;
            switch (actionEnum) {
                case Action::UI_NAV_VERTICAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Up;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Down;
                    }
                    break;

                case Action::UI_NAV_LATERAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Right;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Left;
                    }
                    break;

                case Action::UI_NAV_GROUP:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Tab;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Backtab;
                    }
                    break;

                case Action::UI_NAV_BACK:
                    key = Qt::Key_Escape;
                    break;

                case Action::UI_NAV_SELECT:
                    key = Qt::Key_Return;
                    break;
            }

            if (navAxis) {
                if (lastKey != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyRelease, lastKey, Qt::NoModifier);
                    sendEvent(_offscreenUi->getWindow(), &event);
                    lastKey = Qt::Key_unknown;
                }

                if (key != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(_offscreenUi->getWindow(), &event);
                    lastKey = key;
                }
            } else if (key != Qt::Key_unknown) {
                if (state) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(_offscreenUi->getWindow(), &event);
                } else {
                    QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
                    sendEvent(_offscreenUi->getWindow(), &event);
                }
                return;
            }
        }

        if (action == controller::toInt(controller::Action::RETICLE_CLICK)) {
            auto globalPos = QCursor::pos();
            auto localPos = getWindow()->mapFromGlobal(globalPos);
            if (state) {
                QMouseEvent mousePress(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                sendEvent(_offscreenUi->getWindow(), &mousePress);
                _reticleClickPressed = true;
            } else {
                QMouseEvent mouseRelease(QEvent::MouseButtonRelease, localPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                sendEvent(_offscreenUi->getWindow(), &mouseRelease);
                _reticleClickPressed = false;
            }
            return; // nothing else to do
        }

        if (state) {
            if (action == controller::toInt(controller::Action::TOGGLE_MUTE)) {
            } else if (action == controller::toInt(controller::Action::CYCLE_CAMERA)) {
            } else if (action == controller::toInt(controller::Action::CONTEXT_MENU)) {
            } else if (action == controller::toInt(controller::Action::RETICLE_X)) {
                auto oldPos = QCursor::pos();
                auto newPos = oldPos;
                newPos.setX(oldPos.x() + state);
                QCursor::setPos(newPos);


                // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
                // remove it after we're done
                const float REASONABLE_CHANGE = 50.0f;
                glm::vec2 oldPosG = { oldPos.x(), oldPos.y() };
                glm::vec2 newPosG = { newPos.x(), newPos.y() };
                auto distance = glm::distance(oldPosG, newPosG);
                if (distance > REASONABLE_CHANGE) {
//                    qDebug() << "Action::RETICLE_X... UNREASONABLE CHANGE! distance:" << distance << " oldPos:" << oldPosG << " newPos:" << newPosG;
                }

            } else if (action == controller::toInt(controller::Action::RETICLE_Y)) {
                auto oldPos = QCursor::pos();
                auto newPos = oldPos;
                newPos.setY(oldPos.y() + state);
                QCursor::setPos(newPos);

                // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
                // remove it after we're done
                const float REASONABLE_CHANGE = 50.0f;
                glm::vec2 oldPosG = { oldPos.x(), oldPos.y() };
                glm::vec2 newPosG = { newPos.x(), newPos.y() };
                auto distance = glm::distance(oldPosG, newPosG);
                if (distance > REASONABLE_CHANGE) {
//                    qDebug() << "Action::RETICLE_Y... UNREASONABLE CHANGE! distance:" << distance << " oldPos:" << oldPosG << " newPos:" << newPosG;
                }
            }
        }
    });

    // A new controllerInput device used to reflect current values from the application state
    _applicationStateDevice = std::make_shared<controller::StateController>();

    _applicationStateDevice->addInputVariant(QString("InHMD"), controller::StateController::ReadLambda([this]() -> float {
        return (float)(getActiveDisplayPlugin()->isHmd());
    }));
    _applicationStateDevice->addInputVariant(QString("NavigationFocused"), controller::StateController::ReadLambda([this]() -> float {
        return _offscreenUi->navigationFocused() ? 1.0 : 0.0;
    }));
    userInputMapper->registerDevice(_applicationStateDevice);
    userInputMapper->loadDefaultMapping(userInputMapper->getStandardDeviceID());
}

void UiApplication::cleanupBeforeQuit() {
    _applicationStateDevice.reset();
}

UiApplication::~UiApplication() {
}

void UiApplication::initializeUI() {
    MessageDialog::registerType();
    VrMenu::registerType();

    _offscreenUi = new OffscreenUi();
    _offscreenUi->create(_offscreenContext->getContext());
    _offscreenUi->setProxyWindow(getWindow());
    _offscreenUi->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    _offscreenUi->load("Root.qml");
    _offscreenUi->load("RootMenu.qml");

    auto rootContext = _offscreenUi->getRootContext();
    auto engine = rootContext->engine();
    connect(engine, &QQmlEngine::quit, [] {
        qApp->quit();
    });

    rootContext->setContextProperty("Paths", DependencyManager::get<PathUtils>().data());
    getWindow()->installEventFilter(_offscreenUi);

    connect(_offscreenUi, &OffscreenUi::textureUpdated, this, [this](GLuint textureId) {
        _uiTexture = textureId;
        _offscreenUi->lockTexture(_uiTexture);
        auto displayPlugin = getActiveDisplayPlugin();
        if (displayPlugin) {
            displayPlugin->submitOverlayTexture(_uiTexture, uvec2());
        }
    });

    VrMenu::load();
    VrMenu::executeQueuedLambdas();
    _offscreenUi->resume();

    // This will set up the input plugins UI
    _activeInputPlugins.clear();
    updateInputModes();
}

void UiApplication::resizeGL() {
    PluginApplication::resizeGL();

    if (nullptr == _displayPlugin) {
        return;
    }


    // FIXME the aspect ratio for stereo displays is incorrect based on this.
    float aspectRatio = _displayPlugin->getRecommendedAspectRatio();
    auto uiSize = _displayPlugin->getRecommendedUiSize();
    // Bit of a hack since there's no device pixel ratio change event I can find.
    static qreal lastDevicePixelRatio = 0;
    qreal devicePixelRatio = getWindow()->devicePixelRatio();
    if (_offscreenUi->size() != fromGlm(uiSize) || devicePixelRatio != lastDevicePixelRatio) {
        _offscreenUi->resize(fromGlm(uiSize));
        _offscreenContext->makeCurrent();
        lastDevicePixelRatio = devicePixelRatio;
    }
}
