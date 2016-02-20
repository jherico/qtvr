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
#include <QAbstractListModel>
#include "Cache.h"

namespace shadertoy  {

    class Model : public QAbstractListModel {
        Q_OBJECT;
        Q_PROPERTY(QStringList shaderIds READ shaderIds WRITE setShaderIds);
    public:
        enum ShaderRoles {
            IdRole = Qt::UserRole + 1,
            ShaderRole,
            SearchRole,
            NameRole,
        };

        Model();
        void setShaderIds(const QStringList& ids);
        QStringList shaderIds();

    protected:
        int rowCount(const QModelIndex & parent = QModelIndex()) const override;
        QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;

    public:
        Cache* _cache { nullptr };
        QStringList _currentIds;
    };

}
