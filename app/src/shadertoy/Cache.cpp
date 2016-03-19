#include "Cache.h"

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
    auto shader = Shader::parseString(shaderJson, this);
    if (!shader) {
        return QVariant();
    }
    _shadersById[shader->info->id] = shader;
    return QVariant::fromValue(shader);
}

//QStringList Cache::queryShaders(const QString& query, const QVariantMap& parameters) const {
//    qDebug() << "Queried for " << query << " and parameters " << parameters;
//    QString sort = parameters["sort"].toString();
//    if (!_sortedIds.contains(sort)) {
//        sort = "";
//    }
//    QStringList keys = _sortedIds[sort];
//    qDebug() << "Starting size " << keys.size() << " items";
//
//    auto forwardIterator = keys.begin();
//    //const QString filter = parameters["filter"].toString();
//    //if (!filter.isEmpty() && filter != "none") {
//    //    forwardIterator = std::remove_if(forwardIterator, keys.end(), [&](const QString& shaderId){
//    //        return _shaders[shaderId].matchFilter(filter);
//    //    });
//    //}
//
//    //if (!query.isEmpty()) {
//    //    QRegularExpression queryRe("\\b" + query, QRegularExpression::CaseInsensitiveOption);
//    //    forwardIterator = std::remove_if(forwardIterator, keys.end(), [&](const QString& shaderId){
//    //        return _shaders[shaderId].matchQuery(queryRe);
//    //    });
//    //}
//
//    const int max = parameters["num"].toInt();
//
//    QStringList resultSet;
//    while (forwardIterator != keys.end() && resultSet.size() < max) {
//        QString key = *forwardIterator;
//        resultSet << key;
//        ++forwardIterator;
//    }
//    qDebug() << "Result size " << resultSet.size() << " items";
//    return resultSet;
//}
//struct Item {
//    Item(const QString& id = "");
//    Item(const Item& o);
//    Item& operator=(const Item& o);
//    bool load(const QString& basePath);

//    bool matchQuery(const QRegularExpression& re) const;
//    bool matchFilter(const QString& filter) const;

//    QString id;
//    QVariant shader;
//    QVariant info;
//    QString textSearch;
//    QSet<QString> tags;
//};

//Cache::Item::Item(const QString& id) : id(id) {}
//Cache::Item::Item(const Cache::Item& o) { *this = o; }
//
//Cache::Item& Cache::Item::operator=(const Cache::Item& o) {
//    id = o.id;
//    info = o.info;
//    tags = o.tags;
//    shader = o.shader;
//    textSearch = o.textSearch;
//    return *this;
//}
//
//bool Cache::Item::load(const QString& basePath) {
//    auto json = fromFile(basePath + "/" + id + ".json");
//    auto shaderValue = json.object().value("Shader");
//    if (!shaderValue.isObject()) {
//        return false;
//    }
//
//    auto shaderJson = shaderValue.toObject();
//    shader = shaderJson.toVariantMap();
//
//
//    auto infoValue = shaderJson.value("info");
//    if (!infoValue.isObject()) {
//        return false;
//    }
//
//    auto infoObject = infoValue.toObject();
//    info = infoObject.toVariantMap();
//    textSearch += infoObject.value("name").toString().toLower() + "\n";
//    textSearch += infoObject.value("description").toString() + "\n";
//    textSearch += infoObject.value("username").toString() + "\n";
//    auto tagsJson = infoObject.value("tags").toArray();
//    for (const auto& tagJson : tagsJson) {
//        tags.insert(tagJson.toString());
//    }
//    return true;
//}
//
//bool Cache::Item::matchQuery(const QRegularExpression& re) const {
//    auto matches = re.globalMatch(textSearch);
//    return matches.hasNext();
//}
//
//bool Cache::Item::matchFilter(const QString& filter) const {
//    return tags.contains(filter);
//}
//Shader* Cache::parseShader(const QString& shader) {
//    return processShaderJson(fromString(shader));
//}
