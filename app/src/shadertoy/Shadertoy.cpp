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

#include "Shadertoy.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVariant>
#include <QtCore/QRegularExpression>

#include <QtXml/QDomDocument>

inline QByteArray readFileToByteArray(const QString & fileName) {
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

QJsonValue path(const QJsonValue & parent, std::initializer_list<QVariant> elements) {
    QJsonValue current = parent;
    std::for_each(elements.begin(), elements.end(), [&](const QVariant & element) {
        if (current.isObject()) {
            QString path = element.toString();
            current = current.toObject().value(path);
        } else if (current.isArray()) {
            int offset = element.toInt();
            current = current.toArray().at(offset);
        } else {
            qWarning() << "Unable to continue";
            current = QJsonValue();
        }
    });
    return current;
}

const char* const Shadertoy::UNIFORM_CHANNELS[MAX_CHANNELS] = {
    "iChannel0",
    "iChannel1",
    "iChannel2",
    "iChannel3",
};


const QRegularExpression Shadertoy::VR_MARKER("\\bmainVR\\b");
