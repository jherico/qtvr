/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"];
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
#include <QVariant>


bool ShaderInfo::parse(const QVariant& var) {
    auto varMap = var.toMap();
    name = varMap["name"].toString();
    username = varMap["username"].toString();
    id = varMap["id"].toString();
    auto dateTime = QString(varMap["date"].toString()).toInt();
    date = QDateTime::fromTime_t(dateTime);
    viewed = varMap["viewed"].toInt();
    description = varMap["description"].toString();
    likes = varMap["likes"].toInt();
    published = varMap["published"].toInt();
    flags = varMap["flags"].toInt();
    for (auto tagValue : varMap["tags"].toList()) {
        tags.push_back(tagValue.toString());
    }
    return true;
}

