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
#include <QtCore/QStringList>
#include <QtCore/QDateTime>

class QJsonValue;

class ShaderInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id MEMBER id);
    Q_PROPERTY(QString name MEMBER name);
    Q_PROPERTY(QString username MEMBER username);
    Q_PROPERTY(QString description MEMBER description);
    Q_PROPERTY(QDateTime date MEMBER date);
    Q_PROPERTY(QStringList tags MEMBER tags);
    Q_PROPERTY(int viewed MEMBER viewed);
    Q_PROPERTY(int likes MEMBER likes);
    Q_PROPERTY(int flags MEMBER flags);
    Q_PROPERTY(int published MEMBER published);

public:
    ShaderInfo(QObject* parent = nullptr) : QObject(parent) {}
    QString id;
    QString name;
    QString username;
    QString description;
    QDateTime date;
    QStringList tags;
    int viewed{ 0 };
    int likes{ 0 };
    int flags{ 0 };
    int published{ 0 };
    bool parse(const QJsonValue& shader);
};

