#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QSet>

#include "types/Shader.h"

namespace shadertoy {

    class Cache : public QObject {
        Q_OBJECT
    public:
        Cache(QObject* parent = nullptr);

        Q_INVOKABLE QStringList getShaderList() const;
        Q_INVOKABLE void setShaderList(const QString& shaderListJson);
        Q_INVOKABLE bool hasShader(const QString& shaderId) const;
        Q_INVOKABLE QVariant getShader(const QString& shaderId) const;
        Q_INVOKABLE QVariant setShader(const QString& shaderId, const QString& shaderJson);

    private:
        const QString _basePath;
        QHash<QString, Shader*> _shadersById;
        QStringList _shaderIds;
    };

}