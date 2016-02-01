#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QSet>

class Cache : public QObject {
    Q_OBJECT
public:
    Cache(QObject* parent = nullptr) : QObject(parent) {}
    Cache(const QString& basePath, QObject* parent = nullptr);
    Q_INVOKABLE QStringList fetchShaderList() const;
    Q_INVOKABLE QVariant fetchShader(const QString& shaderId) const;
    Q_INVOKABLE QVariant fetchShaderInfo(const QString& shaderId) const;
    Q_INVOKABLE QStringList queryShaders(const QString& query, const QVariantMap& parameters) const;

private:

    struct Item {
        Item(const QString& id = "");
        Item(const Item& o);
        Item& operator=(const Item& o);
        bool load(const QString& basePath);

        bool matchQuery(const QRegularExpression& re) const;
        bool matchFilter(const QString& filter) const;

        QString id;
        QVariant shader;
        QVariant info;
        QString textSearch;
        QSet<QString> tags;
    };

    const QString _basePath;
    QHash<QString, Item> _shaders;
    QHash<QString, QStringList> _sortedIds;
};

