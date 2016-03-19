/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#pragma once
#include <QtCore/QObject>
#include <QQmlListProperty>

#include "ShaderInfo.h"
#include "Renderpass.h"

class QJsonValue;

class Shader : public QObject {
    Q_OBJECT;
    Q_PROPERTY(ShaderInfo* info MEMBER info CONSTANT);
    Q_PROPERTY(QQmlListProperty<Renderpass> renderpass READ renderpass CONSTANT);
public:
    static Shader* parseJson(const QJsonValue& var, QObject* parent = nullptr);
    static Shader* parseString(const QString& json, QObject* parent = nullptr);
    static Shader* parseFile(const QString& json, QObject* parent = nullptr);

    Shader(QObject* parent = nullptr) : QObject(parent) {}
    ShaderInfo* info{ nullptr };
    bool parse(const QVariantMap& data);
    QQmlListProperty<Renderpass> renderpass();
    QList<Renderpass*> _renderpass;
};
