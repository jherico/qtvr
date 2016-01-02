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

bool Renderpass::parse(const QVariant& var) {
    auto varMap = var.toMap();
    code = varMap["code"].toString();
    
    auto typeValue = varMap["type"].toString();
    if (typeValue == "image") {
        output = IMAGE;
    } else if (typeValue == "buffer") {
        output = BUFFER_A;
    } else if (typeValue == "sound") {
        output = SOUND;
    } else {
        return false;
    }

    for (auto inputValue : varMap["inputs"].toList()) {
        auto input = new Input(this);
        _inputs.push_back(input);
        if (!input->parse(inputValue)) {
            return false;
        }
    }
    return true;
}
