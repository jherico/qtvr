//
//  ResourceManager.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceManager.h"

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "ResourceRequest.h"
#include "SharedUtil.h"

ResourceManager::PrefixMap ResourceManager::_prefixMap;
QMutex ResourceManager::_prefixMapLock;

void ResourceManager::setUrlPrefixOverride(const QString& prefix, const QString& replacement) {
    QMutexLocker locker(&_prefixMapLock);
    _prefixMap[prefix] = replacement;
}

QString ResourceManager::normalizeURL(const QString& urlString) {
    QString result = urlString;
    QMutexLocker locker(&_prefixMapLock);
    foreach(const auto& entry, _prefixMap) {
        const auto& prefix = entry.first;
        const auto& replacement = entry.second;
        if (result.startsWith(prefix)) {
            result.replace(0, prefix.size(), replacement);
        }
    }
    return result;
}

QUrl ResourceManager::normalizeURL(const QUrl& originalUrl) {
    QUrl url = QUrl(normalizeURL(originalUrl.toString()));
    auto scheme = url.scheme();
    if (!(scheme == URL_SCHEME_FILE || scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP)) {
        // check the degenerative file case: on windows we can often have urls of the form c:/filename
        // this checks for and works around that case.
        QUrl urlWithFileScheme{ URL_SCHEME_FILE + ":///" + url.toString() };
        if (!urlWithFileScheme.toLocalFile().isEmpty()) {
            return urlWithFileScheme;
        }
    }
    return url;
}



class HTTPResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    HTTPResourceRequest(QObject* parent, const QUrl& url) : ResourceRequest(parent, url) {}
    ~HTTPResourceRequest() {
        if (_reply) {
            clearReply();
        }
    }

protected:
    virtual void doSend() override {
        QNetworkRequest networkRequest(_url);
        if (_cacheEnabled) {
            networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        } else {
            networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        }

        static QNetworkAccessManager instance;
        _reply = instance.get(networkRequest);

        connect(_reply, &QNetworkReply::finished, this, &HTTPResourceRequest::onRequestFinished);
        connect(_reply, &QNetworkReply::downloadProgress, this, &HTTPResourceRequest::onDownloadProgress);
        connect(&_sendTimer, &QTimer::timeout, this, &HTTPResourceRequest::onTimeout);

        static const int TIMEOUT_MS = 10000;
        _sendTimer.setSingleShot(true);
        _sendTimer.start(TIMEOUT_MS);
    }

    void clearReply() {
        _reply->disconnect(this);
        _reply->deleteLater();
        _reply = nullptr;
    }

private slots:
    void onTimeout() {
        Q_ASSERT(_state == InProgress);
        _reply->abort();
        clearReply();

        _result = Timeout;
        _state = Finished;
        emit finished();
    }

    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
        Q_ASSERT(_state == InProgress);

        // We've received data, so reset the timer
        _sendTimer.start();

        emit progress(bytesReceived, bytesTotal);
    }

    void onRequestFinished() {
        Q_ASSERT(_state == InProgress);
        Q_ASSERT(_reply);

        _sendTimer.stop();

        switch (_reply->error()) {
        case QNetworkReply::NoError:
            _data = _reply->readAll();
            _loadedFromCache = _reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
            _result = Success;
            break;
        case QNetworkReply::TimeoutError:
            _result = Timeout;
            break;
        default:
            _result = Error;
            break;
        }
        clearReply();

        _state = Finished;
        emit finished();
    }

private:
    QTimer _sendTimer;
    QNetworkReply* _reply { nullptr };
};

class FileResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    FileResourceRequest(QObject* parent, const QUrl& url) 
        : ResourceRequest(parent, url) {}

protected:
    virtual void doSend() {
        QString filename = _url.toLocalFile();

        // sometimes on windows, we see the toLocalFile() return null,
        // in this case we will attempt to simply use the url as a string
        if (filename.isEmpty()) {
            filename = _url.toString();
        }

        QFile file(filename);
        if (file.exists()) {
            if (file.open(QFile::ReadOnly)) {
                _data = file.readAll();
                _result = ResourceRequest::Success;
            } else {
                _result = ResourceRequest::AccessDenied;
            }
        } else {
            _result = ResourceRequest::NotFound;
        }

        _state = Finished;
        emit finished();
    }
};


ResourceRequest* ResourceManager::createResourceRequest(QObject* parent, const QUrl& url) {
    auto normalizedURL = normalizeURL(url);
    auto scheme = normalizedURL.scheme();
    if (scheme == URL_SCHEME_FILE) {
        return new FileResourceRequest(parent, normalizedURL);
    } else if (scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP) {
        return new HTTPResourceRequest(parent, normalizedURL);
    }

    qDebug() << "Unknown scheme (" << scheme << ") for URL: " << url.url();

    return nullptr;
}

#include "ResourceManager.moc"