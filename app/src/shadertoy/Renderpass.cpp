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

#include "Renderpass.h"
#include <QJsonObject>
#include <QJsonArray>

bool Renderpass::parse(const QJsonValue& renderpass) {
    if (!renderpass.isObject()) {
        return false;
    }

    auto renderpassObject = renderpass.toObject();
    code = renderpassObject.value("code").toString();
    
    auto typeValue = renderpassObject.value("type").toString();
    if (typeValue == "image") {
        type = IMAGE;
    } else if (typeValue == "buffer") {
        type = BUFFER;
    } else if (typeValue == "sound") {
        type = SOUND;
    } else {
        return false;
    }

    auto inputsValue = renderpassObject.value("inputs");
    if (!inputsValue.isArray()) {
        return false;
    }

    for (auto inputValue : inputsValue.toArray()) {
        if (!inputValue.isObject()) {
            return false;
        }

        auto input = new Input(this);
        _inputs.push_back(input);
        if (!input->parse(inputValue)) {
            return false;
        }
    }
    return true;
}
