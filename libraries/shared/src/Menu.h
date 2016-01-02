//
//  Menu.h
//  interface/src
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Menu_h
#define hifi_Menu_h

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QStandardPaths>

#include <QtGui/QKeySequence>

//#include <MenuItemProperties.h>
//#include "DiscoverabilityManager.h"

namespace menu {
    class QmlWrapper {
    protected: 
        QObject* _item { nullptr };
    };

    class Action : public QmlWrapper {
    public:
        using Pointer = std::shared_ptr<Action>;

        void setName(const QString& name);
        QString name();
    };

    class Item;
    using ItemPointer = std::shared_ptr<Item>;
    using ItemList = std::list<ItemPointer>;

    class Group : public QmlWrapper {
    public:
        using Pointer = std::shared_ptr<Group>;
        ItemList items();
        ItemPointer selectedItem();
        void setDefault(ItemPointer item);
    };

    class Group;
    using GroupPointer = std::shared_ptr<Group>;

    class MenuBase : public QmlWrapper {
    public:
        enum Type { MENU, ITEM };
        Type type() const;
        QString name() const;
    };

    class Item : public MenuBase {
    public:
        using Pointer = std::shared_ptr<Item>;
        using List = std::list<Pointer>;
        GroupPointer group() const;

        void trigger();
        bool checked();
        void setChecked(bool checked);
        bool enabled();
        QString group();

    private:
        GroupPointer _group;
    };


    class Menu : public MenuBase {
    public:
        using Pointer = std::shared_ptr<Menu>;
        using List = std::shared_ptr<Menu>;

        static Pointer topMenu();

        Pointer parent();
        Item::List items();
        List menus();

        Pointer addMenu(const QString& menu);
        void removeMenu(const QString& menu);
        bool menuExists(const QString& menu) const;

        void addSeparator();

        Item::Pointer addItem(const QString& item);
        void removeItem(const QString& item);
        bool itemExists(const QString& item) const;
        void triggerItem(const QString& item);
        Action::Pointer getAction(const QString& item);
    };
}


#endif // hifi_Menu_h
