//
//  OffscreenUi.cpp
//  interface/src/render-utils
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenUi.h"

#include <QtCore/QVariant>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>
#include <QtQml/QtQml>

#include <AbstractUriHandler.h>

//#include "FileDialogHelper.h"


// Needs to match the constants in resources/qml/Global.js
class OffscreenFlags : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool navigationFocused READ isNavigationFocused WRITE setNavigationFocused NOTIFY navigationFocusedChanged)

public:

    OffscreenFlags(QObject* parent = nullptr) : QObject(parent) {}
    bool isNavigationFocused() const { return _navigationFocused; }
    void setNavigationFocused(bool focused) {
        if (_navigationFocused != focused) {
            _navigationFocused = focused;
            emit navigationFocusedChanged();
        }
    }

signals:
    void navigationFocusedChanged();

private:
    bool _navigationFocused { false };
};

class UrlHandler : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE bool canHandleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->canAcceptURL(url);
    }

    Q_INVOKABLE bool handleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->acceptURL(url);
    }
  
};

static OffscreenFlags* offscreenFlags { nullptr };

// This hack allows the QML UI to work with keys that are also bound as 
// shortcuts at the application level.  However, it seems as though the 
// bound actions are still getting triggered.  At least for backspace.  
// Not sure why.
//
// However, the problem may go away once we switch to the new menu system,
// so I think it's OK for the time being.
bool OffscreenUi::shouldSwallowShortcut(QEvent* event) {
    Q_ASSERT(event->type() == QEvent::ShortcutOverride);
    QObject* focusObject = getWindow()->focusObject();
    if (focusObject != getWindow() && focusObject != getRootItem()) {
        //qDebug() << "Swallowed shortcut " << static_cast<QKeyEvent*>(event)->key();
        event->accept();
        return true;
    }
    return false;
}

OffscreenUi::OffscreenUi() {
}

void OffscreenUi::create(QOpenGLContext* context) {
    OffscreenQmlSurface::create(context);
    auto rootContext = getRootContext();

    rootContext->setContextProperty("offscreenFlags", offscreenFlags = new OffscreenFlags());
    rootContext->setContextProperty("urlHandler", new UrlHandler());
}

void OffscreenUi::show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setVisible(true);
    }
}

void OffscreenUi::toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // Already loaded?  
    if (item) {
        item->setVisible(!item->isVisible());
        return;
    }

    load(url, f);
    item = getRootItem()->findChild<QQuickItem*>(name);
    if (item && !item->isVisible()) {
        item->setVisible(true);
    }
}

class ModalDialogListener : public QObject {
    Q_OBJECT
    friend class OffscreenUi;

protected:
    ModalDialogListener(QQuickItem* dialog) : _dialog(dialog) {
        if (!dialog) {
            _finished = true;
            return;
        }
        connect(_dialog, SIGNAL(destroyed()), this, SLOT(onDestroyed()));
    }

    ~ModalDialogListener() {
        if (_dialog) {
            disconnect(_dialog);
        }
    }

    virtual QVariant waitForResult() {
        while (!_finished) {
            QCoreApplication::processEvents();
        }
        return _result;
    }

protected slots:
    void onDestroyed() {
        _finished = true;
        disconnect(_dialog);
        _dialog = nullptr;
    }

protected:
    QQuickItem* _dialog;
    bool _finished { false };
    QVariant _result;
};

class MessageBoxListener : public ModalDialogListener {
    Q_OBJECT

    friend class OffscreenUi;
    MessageBoxListener(QQuickItem* messageBox) : ModalDialogListener(messageBox) {
        if (_finished) {
            return;
        }
        connect(_dialog, SIGNAL(selected(int)), this, SLOT(onSelected(int)));
    }

    virtual QMessageBox::StandardButton waitForButtonResult() {
        ModalDialogListener::waitForResult();
        return static_cast<QMessageBox::StandardButton>(_result.toInt());
    }

private slots:
    void onSelected(int button) {
        _result = button;
        _finished = true;
        disconnect(_dialog);
    }
};

QMessageBox::StandardButton OffscreenUi::messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    if (QThread::currentThread() != thread()) {
        QMessageBox::StandardButton result = QMessageBox::StandardButton::NoButton;
        QMetaObject::invokeMethod(this, "messageBox", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QMessageBox::StandardButton, result),
            Q_ARG(QMessageBox::Icon, icon),
            Q_ARG(QString, title),
            Q_ARG(QString, text),
            Q_ARG(QMessageBox::StandardButtons, buttons),
            Q_ARG(QMessageBox::StandardButton, defaultButton));
        return result;
    }

    QVariantMap map;
    map.insert("title", title);
    map.insert("text", text);
    map.insert("icon", icon);
    map.insert("buttons", buttons.operator int());
    map.insert("defaultButton", defaultButton);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "messageBox",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create message box";
        return QMessageBox::StandardButton::NoButton;
    }
    
    QMessageBox::StandardButton resultButton = MessageBoxListener(qvariant_cast<QQuickItem*>(result)).waitForButtonResult();
    qDebug() << "Message box got a result of " << resultButton;
    return resultButton;
}

QMessageBox::StandardButton OffscreenUi::critical(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::information(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::question(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::warning(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}



class InputDialogListener : public ModalDialogListener {
    Q_OBJECT

    friend class OffscreenUi;
    InputDialogListener(QQuickItem* queryBox) : ModalDialogListener(queryBox) {
        if (_finished) {
            return;
        }
        connect(_dialog, SIGNAL(selected(QVariant)), this, SLOT(onSelected(const QVariant&)));
    }

private slots:
    void onSelected(const QVariant& result) {
        _result = result;
        _finished = true;
        disconnect(_dialog);
    }
};

// FIXME many input parameters currently ignored
QString OffscreenUi::getText(void* ignored, const QString & title, const QString & label, QLineEdit::EchoMode mode, const QString & text, bool * ok, Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints) {
    QVariant result = DependencyManager::get<OffscreenUi>()->inputDialog(title, label, text).toString();
    if (ok && result.isValid()) {
        *ok = true;
    }
    return result.toString();
}


QVariant OffscreenUi::inputDialog(const QString& query, const QString& placeholderText, const QString& currentValue) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, "queryBox", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(QString, query),
            Q_ARG(QString, placeholderText),
            Q_ARG(QString, currentValue));
        return result;
    }

    QVariantMap map;
    map.insert("text", query);
    map.insert("placeholderText", placeholderText);
    map.insert("result", currentValue);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "queryBox",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create message box";
        return QVariant();
    }

    return InputDialogListener(qvariant_cast<QQuickItem*>(result)).waitForResult();
}


bool OffscreenUi::navigationFocused() {
    return offscreenFlags->isNavigationFocused();
}

void OffscreenUi::setNavigationFocused(bool focused) {
    offscreenFlags->setNavigationFocused(focused);
}

// FIXME HACK....
// This hack is an attempt to work around the 'offscreen UI can't gain keyboard focus' bug
// https://app.asana.com/0/27650181942747/83176475832393
// The problem seems related to https://bugreports.qt.io/browse/QTBUG-50309 
//
// The workaround seems to be to give some other window (same process or another process doesn't seem to matter)
// focus and then put focus back on the interface main window.  
//
// If I could reliably reproduce this bug I could eventually track down what state change is occuring 
// during the process of the main window losing and then gaining focus, but failing that, here's a 
// brute force way of triggering that state change at application start in a way that should be nearly
// imperceptible to the user.
class KeyboardFocusHack : public QObject {
    Q_OBJECT
public:
    KeyboardFocusHack() {
        Q_ASSERT(_mainWindow);
        QTimer::singleShot(200, [=] {
            _hackWindow = new QWindow();
            _hackWindow->setFlags(Qt::FramelessWindowHint);
            _hackWindow->setGeometry(_mainWindow->x(), _mainWindow->y(), 10, 10);
            _hackWindow->show();
            _hackWindow->requestActivate();
            QTimer::singleShot(200, [=] {
                _hackWindow->hide();
                _hackWindow->deleteLater();
                _hackWindow = nullptr;
                _mainWindow->requestActivate();
                this->deleteLater();
            });
        });
    }

private:
    
    static QWindow* findMainWindow() {
        auto windows = qApp->topLevelWindows();
        QWindow* result = nullptr;
        for (auto window : windows) {
            QVariant isMainWindow = window->property("MainWindow");
            if (!qobject_cast<QQuickWindow*>(window)) {
                result = window;
                break;
            }
        }
        return result;
    }

    QWindow* const _mainWindow { findMainWindow() };
    QWindow* _hackWindow { nullptr };
};

void OffscreenUi::createDesktop(const QUrl& url) {
    if (_desktop) {
        qDebug() << "Desktop already created";
        return;
    }

    

#ifdef DEBUG
    getRootContext()->setContextProperty("DebugQML", QVariant(true));
#else 
    getRootContext()->setContextProperty("DebugQML", QVariant(false));
#endif

    _desktop = dynamic_cast<QQuickItem*>(load(url));
    Q_ASSERT(_desktop);
    getRootContext()->setContextProperty("desktop", _desktop);

    _toolWindow = _desktop->findChild<QQuickItem*>("ToolWindow");

    
    new KeyboardFocusHack();
}

QQuickItem* OffscreenUi::getDesktop() {
    return _desktop;
}

QQuickItem* OffscreenUi::getToolWindow() {
    return _toolWindow;
}

QObject* OffscreenUi::getMenu() {
    return qvariant_cast<QObject*>(_desktop->property("rootMenu"));
}

void OffscreenUi::unfocusWindows() {
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "unfocusWindows");
    Q_ASSERT(invokeResult);
}

void OffscreenUi::toggleMenu(const QPoint& screenPosition) {
    auto virtualPos = mapToVirtualScreen(screenPosition, nullptr);
    QMetaObject::invokeMethod(_desktop, "toggleMenu",  Q_ARG(QVariant, virtualPos));
}


#include "OffscreenUi.moc"
