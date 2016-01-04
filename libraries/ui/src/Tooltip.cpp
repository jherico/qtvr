//
//  Tooltip.cpp
//  libraries/ui/src
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Tooltip.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>

HIFI_QML_DEF(Tooltip)

Tooltip::Tooltip(QQuickItem* parent) : QQuickItem(parent) {
    connect(this, &Tooltip::titleChanged, this, &Tooltip::requestHyperlinkImage);
}

Tooltip::~Tooltip() {

}

void Tooltip::setTitle(const QString& title) {
    if (title != _title) {
        _title = title;
        emit titleChanged();
    }
}

void Tooltip::setDescription(const QString& description) {
    if (description != _description) {
        _description = description;
        emit descriptionChanged();
    }
}

void Tooltip::setImageURL(const QString& imageURL) {
    if (imageURL != _imageURL) {
        _imageURL = imageURL;
        emit imageURLChanged();
    }
}

void Tooltip::setVisible(bool visible) {
    QQuickItem::setVisible(visible);
}

QString Tooltip::showTip(const QString& title, const QString& description) {
    const QString newTipId = QUuid().createUuid().toString();

    Tooltip::show([&](QQmlContext*, QObject* object) {
        object->setObjectName(newTipId);
        object->setProperty("title", title);
        object->setProperty("description", description);
    });

    return newTipId;
}

void Tooltip::closeTip(const QString& tipId) {
    auto rootItem = qApp->getOffscreenUi()->getRootItem();
    QQuickItem* that = rootItem->findChild<QQuickItem*>(tipId);
    if (that) {
        that->deleteLater();
    }
}

void Tooltip::requestHyperlinkImage() {
    if (!_title.isEmpty()) {
        // we need to decide if this is a place name - if so we should ask the API for the associated image
        // and description (if we weren't given one via the entity properties)
        const QString PLACE_NAME_REGEX_STRING = "^[0-9A-Za-z](([0-9A-Za-z]|-(?!-))*[^\\W_]$|$)";

        QRegExp placeNameRegex(PLACE_NAME_REGEX_STRING);
    }
}
