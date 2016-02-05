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

#include "Shader.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QQmlListProperty<Renderpass> Shader::renderpass() {
    return QQmlListProperty<Renderpass>(this, _renderpass);
}

QJsonDocument fromFile(const QString& filePath);

Shader* Shader::read(const QString& file) {
    Shader* result = new Shader();
    if (!result->parse(fromFile(file).object().value("Shader").toObject())) {
        delete result;
        return nullptr;
    }
    return result;
}

bool Shader::parse(const QJsonValue& shader) {
    if (!shader.isObject()) {
        return false;
    }
    auto shaderObject = shader.toObject();
    if (!shaderObject.value("info").isObject() || !shaderObject.value("renderpass").isArray()) {
        return false;
    }

    info = new ShaderInfo(this);
    if (!info->parse(shaderObject.value("info"))) {
        return false;
    }

    auto renderpassArray = shaderObject.value("renderpass").toArray();
    for (auto renderpassValue : renderpassArray) {
        auto rp = new Renderpass(this);
        _renderpass.push_back(rp);
        if (!rp->parse(renderpassValue)) {
            return false;
        }
    }
    return true;
}
