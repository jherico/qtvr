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

#include <shared/JSONHelpers.h>
#include <FileUtils.h>

QQmlListProperty<Renderpass> Shader::renderpass() {
    return QQmlListProperty<Renderpass>(this, _renderpass);
}

Shader* Shader::parseFile(const QString& var, QObject* parent) {
    return parseString(FileUtils::readFileToString(var), parent);
}

Shader* Shader::parseString(const QString& var, QObject* parent) {
    auto doc = jsonFromString(var);
    return parseJson(doc.object().value("Shader"), parent);
}

Shader* Shader::parseJson(const QJsonValue& var, QObject* parent) {
    Shader* result = new Shader(parent);
    if (!result->parse(var.toVariant().toMap())) {
        delete result;
        return nullptr;
    }
    return result;
}

bool Shader::parse(const QVariantMap& varMap) {
    info = new ShaderInfo(this);
    if (!info->parse(varMap["info"])) {
        return false;
    }

    for (auto renderpassValue : varMap["renderpass"].toList()) {
        auto rp = new Renderpass(this);
        _renderpass.push_back(rp);
        if (!rp->parse(renderpassValue)) {
            return false;
        }
    }
    int i = 0;
    for (auto& pass : _renderpass) {
        if (pass->output == Renderpass::BUFFER_A) {
            pass->output = static_cast<Renderpass::Output>(Renderpass::BUFFER_A + i);
            ++i;
        }
    }
    return true;
}
