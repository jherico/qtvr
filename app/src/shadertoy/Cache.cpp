#include "Cache.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include "Shader.h"

QJsonDocument fromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "Could not find file" << filePath;
        return QJsonDocument();
    }
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Could not open file" << filePath;
        return QJsonDocument();
    }
    auto byteArray = file.readAll();
    return QJsonDocument::fromJson(byteArray);
}

Cache::Cache(const QString& basePath, QObject* parent) : QObject(parent), _basePath(basePath) {
    auto doc = fromFile(basePath + "/shadertoys.json");
    auto jsonIds = doc.object().value("Results").toArray();
    for (auto jsonId : jsonIds) {
        auto id = jsonId.toString();
        Item item(id);
        auto shader = Shader::read(basePath + "/" + id + ".json");
        if (item.load(basePath)) {
            _shaders.insert(id, item);
            _shaderIds.push_back(id);
        }
    }
    _sortedIds[""] = _shaders.keys();
    qDebug() << "Loaded " << _shaders.size() << "Shader IDs";
}

QStringList Cache::fetchShaderList() const {
    return _shaders.keys();
}

QVariant Cache::fetchShader(const QString& shaderId) const  {
    if (!_shaders.contains(shaderId)) {
        qDebug() << "No such shader found";
        return QVariant();
    }
    return _shaders[shaderId].shader;
}

 QVariant Cache::fetchShaderInfo(const QString& shaderId) const  {
    if (!_shaders.contains(shaderId)) {
        qDebug() << "No such shader found";
        return QVariant();
    }
    return _shaders[shaderId].info;
}

 QStringList Cache::queryShaders(const QString& query, const QVariantMap& parameters) const {
    qDebug() << "Queried for " << query << " and parameters " << parameters;

    QString sort = parameters["sort"].toString();
    if (!_sortedIds.contains(sort)) {
        sort = "";
    }
    QStringList keys = _sortedIds[sort];
    qDebug() << "Starting size " << keys.size() << " items";

    auto forwardIterator = keys.begin();
    const QString filter = parameters["filter"].toString();
    if (!filter.isEmpty() && filter != "none") {
        forwardIterator = std::remove_if(forwardIterator, keys.end(), [&](const QString& shaderId){
            return _shaders[shaderId].matchFilter(filter);
        });
    }

    if (!query.isEmpty()) {
        QRegularExpression queryRe("\\b" + query, QRegularExpression::CaseInsensitiveOption);
        forwardIterator = std::remove_if(forwardIterator, keys.end(), [&](const QString& shaderId){
            return _shaders[shaderId].matchQuery(queryRe);
        });
    }

    const int max = parameters["num"].toInt();

    QStringList resultSet;
    while (forwardIterator != keys.end() && resultSet.size() < max) {
        QString key = *forwardIterator;
        resultSet << key;
        ++forwardIterator;
    }
    qDebug() << "Result size " << resultSet.size() << " items";
    return resultSet;
}

 Cache::Item::Item(const QString& id) : id(id) {}
 Cache::Item::Item(const Cache::Item& o) { *this = o; }

 Cache::Item& Cache::Item::operator=(const Cache::Item& o) {
     id = o.id;
     info = o.info;
     tags = o.tags;
     shader = o.shader;
     textSearch = o.textSearch;
     return *this;
 }

 bool Cache::Item::load(const QString& basePath) {
     auto json = fromFile(basePath + "/" + id + ".json");
     auto shaderValue = json.object().value("Shader");
     if (!shaderValue.isObject()) {
         return false;
     }

     auto shaderJson = shaderValue.toObject();
     shader = shaderJson.toVariantMap();


     auto infoValue = shaderJson.value("info");
     if (!infoValue.isObject()) {
         return false;
     }

     auto infoObject = infoValue.toObject();
     info = infoObject.toVariantMap();
     textSearch += infoObject.value("name").toString().toLower() + "\n";
     textSearch += infoObject.value("description").toString() + "\n";
     textSearch += infoObject.value("username").toString() + "\n";
     auto tagsJson = infoObject.value("tags").toArray();
     for (const auto& tagJson : tagsJson){
         tags.insert(tagJson.toString());
     }
     return true;
 }

 bool Cache::Item::matchQuery(const QRegularExpression& re) const {
     auto matches = re.globalMatch(textSearch);
     return matches.hasNext();
 }

 bool Cache::Item::matchFilter(const QString& filter) const {
     return tags.contains(filter);
 }
