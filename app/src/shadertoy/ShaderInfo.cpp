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

#include "ShaderInfo.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>


bool ShaderInfo::parse(const QJsonValue& shaderInfo) {
    if (!shaderInfo.isObject()) {
        return false;
    }
    auto shaderInfoObject = shaderInfo.toObject();
    name = shaderInfoObject.value("name").toString();
    username = shaderInfoObject.value("username").toString();
    id = shaderInfoObject.value("id").toString();
    auto dateTime = QString(shaderInfoObject.value("date").toString()).toInt();
    date = QDateTime::fromTime_t(dateTime);
    viewed = shaderInfoObject.value("viewed").toInt();
    description = shaderInfoObject.value("description").toString();
    likes = shaderInfoObject.value("likes").toInt();
    published = shaderInfoObject.value("published").toInt();
    flags = shaderInfoObject.value("flags").toInt();
    auto tagsArray = shaderInfoObject.value("tags").toArray();
    for (auto tagValue : tagsArray) {
        tags.push_back(tagValue.toString());
    }
    return true;
}

