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

#include "ShaderModel.h"
#include <QDebug>

ShaderModel::ShaderModel() {
    _cache = new Cache("C:/Users/austi/git/shadertoys/");
}

int ShaderModel::rowCount(const QModelIndex & parent) const {
    return _cache->_shaderIds.size();
}

QVariant ShaderModel::data(const QModelIndex & index, int role) const {
    auto shader = _cache->_shaders[_cache->_shaderIds[index.row()]];
    
    switch (role) {
    case NameRole:
        return shader.id; 
    case IdRole:
        return shader.id;
    default:
        qDebug() << "Huh?";
    }
    return QVariant("Hello");
}

QHash<int, QByteArray> ShaderModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IdRole] = "shaderId";
    return roles;
}
