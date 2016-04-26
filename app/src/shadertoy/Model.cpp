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
#if 0
#include "Model.h"
#include <QDebug>
#include <QNetworkAccessManager>

using namespace shadertoy;

Model::Model() {
    _cache = new Cache();
    _currentIds = _cache->getShaderList();
}

QStringList Model::shaderIds() {
    return _currentIds;
}

void Model::setShaderIds(const QStringList& ids) {
    beginResetModel();
    _currentIds = ids;
    endResetModel();
}

int Model::rowCount(const QModelIndex & parent) const {
    return _currentIds.size();
}

QVariant Model::data(const QModelIndex & index, int role) const {
    const auto& id = _currentIds[index.row()];
    auto shaderVar = _cache->getShader(id);
    Shader* shader = qvariant_cast<Shader*>(shaderVar);
    switch (role) {
    case IdRole:
        return id;
    case ShaderRole:
        return shaderVar;
    default:
    case NameRole:
    case SearchRole:
        return shader->info->name;
    }
}

QHash<int, QByteArray> Model::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "modelShaderId";
    roles[ShaderRole] = "modelShader";
    roles[SearchRole] = "modelSearch";
    roles[NameRole] = "modelName";
    return roles;
}
#endif
