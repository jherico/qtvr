//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Menu.h"

#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtWidgets/QShortcut>

#include <SettingHandle.h>

#include "../OffscreenUi.h"

#include "Logging.h"

using namespace ui;

Menu::Menu() {}

void Menu::toggleAdvancedMenus() {
    setGroupingIsVisible("Advanced", !getGroupingIsVisible("Advanced"));
}

void Menu::toggleDeveloperMenus() {
    setGroupingIsVisible("Developer", !getGroupingIsVisible("Developer"));
}

void Menu::loadSettings() {
    scanMenuBar(&Menu::loadAction);
}

void Menu::saveSettings() {
    scanMenuBar(&Menu::saveAction);
}

void Menu::loadAction(Settings& settings, QAction& action) {
    if (action.isChecked() != settings.value(action.text(), action.isChecked()).toBool()) {
        action.trigger();
    }
}

void Menu::saveAction(Settings& settings, QAction& action) {
    settings.setValue(action.text(),  action.isChecked());
}

void Menu::scanMenuBar(settingsAction modifySetting) {
    Settings settings;
    foreach (QMenu* menu, findChildren<QMenu*>()) {
        scanMenu(*menu, modifySetting, settings);
    }
}

void Menu::scanMenu(QMenu& menu, settingsAction modifySetting, Settings& settings) {
    settings.beginGroup(menu.title());
    foreach (QAction* action, menu.actions()) {
        if (action->menu()) {
            scanMenu(*action->menu(), modifySetting, settings);
        } else if (action->isCheckable()) {
            modifySetting(settings, *action);
        }
    }
    settings.endGroup();
}

void Menu::addDisabledActionAndSeparator(MenuWrapper* destinationMenu, const QString& actionName, 
                                            int menuItemLocation, const QString& grouping) {
    QAction* actionBefore = NULL;
    QAction* separator;
    QAction* separatorText;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }
    if (actionBefore) {
        separator = new QAction("",destinationMenu);
        destinationMenu->insertAction(actionBefore, separator);
        separator->setSeparator(true);

        separatorText = new QAction(actionName,destinationMenu);
        separatorText->setEnabled(false);
        destinationMenu->insertAction(actionBefore, separatorText);

    } else {
        separator = destinationMenu->addSeparator();
        separatorText = destinationMenu->addAction(actionName);
        separatorText->setEnabled(false);
    }

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << separator;
        _groupingActions[grouping] << separatorText;
        bool isVisible = getGroupingIsVisible(grouping);
        separator->setVisible(isVisible);
        separatorText->setVisible(isVisible);
    }
}

QAction* Menu::addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             const QObject* receiver,
                                             const char* member,
                                             QAction::MenuRole role,
                                             int menuItemLocation, 
                                             const QString& grouping) {
    QAction* action = NULL;
    QAction* actionBefore = NULL;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }

    if (!actionBefore) {
        if (receiver && member) {
            action = destinationMenu->addAction(actionName, receiver, member, shortcut);
        } else {
            action = destinationMenu->addAction(actionName);
            action->setShortcut(shortcut);
        }
    } else {
        action = new QAction(actionName, destinationMenu);
        action->setShortcut(shortcut);
        destinationMenu->insertAction(actionBefore, action);

        if (receiver && member) {
            connect(action, SIGNAL(triggered()), receiver, member);
        }
    }
    action->setMenuRole(role);

    _actionHash.insert(actionName, action);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

QAction* Menu::addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                             QAction* action,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             QAction::MenuRole role,
                                             int menuItemLocation, 
                                             const QString& grouping) {
    QAction* actionBefore = NULL;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }

    if (!actionName.isEmpty()) {
        action->setText(actionName);
    }

    if (shortcut != 0) {
        action->setShortcut(shortcut);
    }

    if (role != QAction::NoRole) {
        action->setMenuRole(role);
    }

    if (!actionBefore) {
        destinationMenu->addAction(action);
    } else {
        destinationMenu->insertAction(actionBefore, action);
    }

    _actionHash.insert(action->text(), action);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

QAction* Menu::addCheckableActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                                      const QString& actionName,
                                                      const QKeySequence& shortcut,
                                                      const bool checked,
                                                      const QObject* receiver,
                                                      const char* member,
                                                      int menuItemLocation, 
                                                      const QString& grouping) {

    QAction* action = addActionToQMenuAndActionHash(destinationMenu, actionName, shortcut, receiver, member,
                                                        QAction::NoRole, menuItemLocation);
    action->setCheckable(true);
    action->setChecked(checked);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

void Menu::removeAction(MenuWrapper* menu, const QString& actionName) {
    auto action = _actionHash.value(actionName);
    menu->removeAction(action);
    _actionHash.remove(actionName);
    for (auto& grouping : _groupingActions) {
        grouping.remove(action);
    }
}

void Menu::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "setIsOptionChecked", Qt::BlockingQueuedConnection,
                    Q_ARG(const QString&, menuOption),
                    Q_ARG(bool, isChecked));
        return;
    }
    QAction* menu = _actionHash.value(menuOption);
    if (menu && menu->isCheckable()) {
        auto wasChecked = menu->isChecked();
        if (wasChecked != isChecked) {
            menu->trigger();
        }
    }
}

bool Menu::isOptionChecked(const QString& menuOption) const {
    const QAction* menu = _actionHash.value(menuOption);
    if (menu) {
        return menu->isChecked();
    }
    return false;
}

void Menu::triggerOption(const QString& menuOption) {
    QAction* action = _actionHash.value(menuOption);
    if (action) {
        action->trigger();
    } else {
        qCDebug(uiLogging) << "NULL Action for menuOption '" << menuOption << "'";
    }
}

QAction* Menu::getActionForOption(const QString& menuOption) {
    return _actionHash.value(menuOption);
}

QAction* Menu::getActionFromName(const QString& menuName, MenuWrapper* menu) {
    QList<QAction*> menuActions;
    if (menu) {
        menuActions = menu->actions();
    } else {
        menuActions = actions();
    }

    foreach (QAction* menuAction, menuActions) {
        QString actionText = menuAction->text();
        if (menuName == menuAction->text()) {
            return menuAction;
        }
    }
    return NULL;
}

MenuWrapper* Menu::getSubMenuFromName(const QString& menuName, MenuWrapper* menu) {
    QAction* action = getActionFromName(menuName, menu);
    if (action) {
        return _backMap[action->menu()];
    }
    return NULL;
}

MenuWrapper* Menu::getMenuParent(const QString& menuName, QString& finalMenuPart) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    MenuWrapper* menu = NULL;
    foreach (QString menuTreePart, menuTree) {
        parent = menu;
        finalMenuPart = menuTreePart.trimmed();
        menu = getSubMenuFromName(finalMenuPart, parent);
        if (!menu) {
            break;
        }
    }
    return parent;
}

MenuWrapper* Menu::getMenu(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    MenuWrapper* menu = NULL;
    int item = 0;
    foreach (QString menuTreePart, menuTree) {
        menu = getSubMenuFromName(menuTreePart.trimmed(), parent);
        if (!menu) {
            break;
        }
        parent = menu;
        item++;
    }
    return menu;
}

QAction* Menu::getMenuAction(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    QAction* action = NULL;
    foreach (QString menuTreePart, menuTree) {
        action = getActionFromName(menuTreePart.trimmed(), parent);
        if (!action) {
            break;
        }
        parent = _backMap[action->menu()];
    }
    return action;
}

int Menu::findPositionOfMenuItem(MenuWrapper* menu, const QString& searchMenuItem) {
    int position = 0;
    foreach(QAction* action, menu->actions()) {
        if (action->text() == searchMenuItem) {
            return position;
        }
        position++;
    }
    return UNSPECIFIED_POSITION; // not found
}

int Menu::positionBeforeSeparatorIfNeeded(MenuWrapper* menu, int requestedPosition) {
    QList<QAction*> menuActions = menu->actions();
    if (requestedPosition > 1 && requestedPosition < menuActions.size()) {
        QAction* beforeRequested = menuActions[requestedPosition - 1];
        if (beforeRequested->isSeparator()) {
            requestedPosition--;
        }
    }
    return requestedPosition;
}

bool Menu::_isSomeSubmenuShown = false;

MenuWrapper* Menu::addMenu(const QString& menuName, const QString& grouping) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* addTo = NULL;
    MenuWrapper* menu = NULL;
    foreach (QString menuTreePart, menuTree) {
        menu = getSubMenuFromName(menuTreePart.trimmed(), addTo);
        if (!menu) {
            if (!addTo) {
                menu = new MenuWrapper(*this, QMenuBar::addMenu(menuTreePart.trimmed()));
            } else {
                menu = addTo->addMenu(menuTreePart.trimmed());
            }
        }
        addTo = menu;
    }

    if (isValidGrouping(grouping)) {
        auto action = getMenuAction(menuName);
        if (action) {
            _groupingActions[grouping] << action;
            action->setVisible(getGroupingIsVisible(grouping));
        }
    }

    QMenuBar::repaint();

    // hook our show/hide for popup menus, so we can keep track of whether or not one
    // of our submenus is currently showing.
    connect(menu->_realMenu, &QMenu::aboutToShow, []() { _isSomeSubmenuShown = true; });
    connect(menu->_realMenu, &QMenu::aboutToHide, []() { _isSomeSubmenuShown = false; });

    return menu;
}

void Menu::removeMenu(const QString& menuName) {
    QAction* action = getMenuAction(menuName);

    // only proceed if the menu actually exists
    if (action) {
        QString finalMenuPart;
        MenuWrapper* parent = getMenuParent(menuName, finalMenuPart);
        if (parent) {
            parent->removeAction(action);
        } else {
            QMenuBar::removeAction(action);
        }

        QMenuBar::repaint();
    }
}

bool Menu::menuExists(const QString& menuName) {
    QAction* action = getMenuAction(menuName);

    // only proceed if the menu actually exists
    if (action) {
        return true;
    }
    return false;
}

void Menu::addSeparator(const QString& menuName, const QString& separatorName, const QString& grouping) {
    MenuWrapper* menuObj = getMenu(menuName);
    if (menuObj) {
        addDisabledActionAndSeparator(menuObj, separatorName);
    }
}

void Menu::removeSeparator(const QString& menuName, const QString& separatorName) {
    MenuWrapper* menu = getMenu(menuName);
    bool separatorRemoved = false;
    if (menu) {
        int textAt = findPositionOfMenuItem(menu, separatorName);
        QList<QAction*> menuActions = menu->actions();
        QAction* separatorText = menuActions[textAt];
        if (textAt > 0 && textAt < menuActions.size()) {
            QAction* separatorLine = menuActions[textAt - 1];
            if (separatorLine) {
                if (separatorLine->isSeparator()) {
                    menu->removeAction(separatorText);
                    menu->removeAction(separatorLine);
                    separatorRemoved = true;
                }
            }
        }
    }
    if (separatorRemoved) {
        QMenuBar::repaint();
    }
}

void Menu::removeMenuItem(const QString& menu, const QString& menuitem) {
    MenuWrapper* menuObj = getMenu(menu);
    if (menuObj) {
        removeAction(menuObj, menuitem);
        QMenuBar::repaint();
    }
}

bool Menu::menuItemExists(const QString& menu, const QString& menuitem) {
    QAction* menuItemAction = _actionHash.value(menuitem);
    if (menuItemAction) {
        return (getMenu(menu) != NULL);
    }
    return false;
}

bool Menu::getGroupingIsVisible(const QString& grouping) {
    if (grouping.isEmpty() || grouping.isNull()) {
        return true;
    }
    if (_groupingVisible.contains(grouping)) {
        return _groupingVisible[grouping];
    }
    return false;
}

void Menu::setGroupingIsVisible(const QString& grouping, bool isVisible) {
    // NOTE: Default grouping always visible
    if (grouping.isEmpty() || grouping.isNull()) {
        return;
    }
    _groupingVisible[grouping] = isVisible;

    for (auto action: _groupingActions[grouping]) {
        action->setVisible(isVisible);
    }

    QMenuBar::repaint();
}

void Menu::addActionGroup(const QString& groupName, const QStringList& actionList, const QString& selected, QObject* receiver, const char* slot) {
    auto menu = addMenu(groupName);
    
    QActionGroup* actionGroup = new QActionGroup(menu);
    actionGroup->setExclusive(true);
    
    for (auto action : actionList) {
        auto item = addCheckableActionToQMenuAndActionHash(menu, action, 0, action == selected, receiver, slot);
        actionGroup->addAction(item);
    }
    
    QMenuBar::repaint();
}

void Menu::removeActionGroup(const QString& groupName) {
    removeMenu(groupName);
}

MenuWrapper::MenuWrapper(ui::Menu& rootMenu, QMenu* menu) : _rootMenu(rootMenu), _realMenu(menu) {
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->addMenu(menu);
    });
#endif
    _rootMenu._backMap[menu] = this;
}

QList<QAction*> MenuWrapper::actions() {
    return _realMenu->actions();
}

MenuWrapper* MenuWrapper::addMenu(const QString& menuName) {
    return new MenuWrapper(_rootMenu, _realMenu->addMenu(menuName));
}

void MenuWrapper::setEnabled(bool enabled) {
    _realMenu->setEnabled(enabled);
}

QAction* MenuWrapper::addSeparator() {
    QAction* action = _realMenu->addSeparator();
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->addSeparator(_realMenu);
    });
#endif
    return action;
}

void MenuWrapper::addAction(QAction* action) {
    _realMenu->addAction(action);
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
#endif
}

QAction* MenuWrapper::addAction(const QString& menuName) {
    QAction* action = _realMenu->addAction(menuName);
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
#endif
    return action;
}

QAction* MenuWrapper::addAction(const QString& menuName, const QObject* receiver, const char* member, const QKeySequence& shortcut) {
    QAction* action = _realMenu->addAction(menuName, receiver, member, shortcut);
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
#endif
    return action;
}

void MenuWrapper::removeAction(QAction* action) {
    _realMenu->removeAction(action);
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->removeAction(action);
    });
#endif
}

void MenuWrapper::insertAction(QAction* before, QAction* action) {
    _realMenu->insertAction(before, action);
#if 0
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->addMenuInitializer([=](VrMenu* vrMenu) {
        vrMenu->insertAction(before, action);
    });
#endif
}

