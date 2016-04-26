#include "Cache.h"

#if 0
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QFile>
#include <QtCore/QDebug>

#include <shared/JSONHelpers.h>
#include <FileUtils.h>

#include "types/Shader.h"

using namespace shadertoy;

Cache::Cache(QObject* parent) : QObject(parent), _basePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/shadertoys/") {
    if (QFile::exists(_basePath + "/shadertoys.json")) {
        setShaderList(FileUtils::readFileToString(_basePath + "/shadertoys.json"));
    }
}

QStringList Cache::getShaderList() const {
    return _shaderIds;
}

void Cache::setShaderList(const QString& shaderListJson) {
    //FileUtils::writeToFile(_basePath + "/shadertoys.json", shaderListJson);
    auto doc = jsonFromString(shaderListJson);
    auto jsonIds = doc.object().value("Results").toArray();
    for (auto jsonId : jsonIds) {
        _shaderIds << jsonId.toString();
    }
    qDebug() << "Loaded " << _shaderIds.size() << "Shader IDs";
}

bool Cache::hasShader(const QString& shaderId) const {
    return _shaderIds.contains(shaderId);
}

QVariant Cache::getShader(const QString& shaderId) const {
    if (!_shaderIds.contains(shaderId)) {
        return QVariant();
    }

    if (!_shadersById.contains(shaderId)) {
        const QString fileName = _basePath + "/" + shaderId + ".json";
        if (QFile::exists(fileName)) {
            return const_cast<Cache*>(this)->setShader(shaderId, FileUtils::readFileToString(fileName));
        }
    }

    if (_shadersById.contains(shaderId)) {
        return QVariant::fromValue(_shadersById[shaderId]);
    }

    return QVariant();
}

QVariant Cache::setShader(const QString& shaderId, const QString& shaderJson) {
    auto shader = new Shader(this);
    auto doc = jsonFromString(shaderJson);
    if (!shader->parse(doc.object().value("Shader"))) {
        delete shader;
        return QVariant();
    }
    _shadersById[shader->info->id] = shader;
    return QVariant::fromValue(shader);
}

#endif