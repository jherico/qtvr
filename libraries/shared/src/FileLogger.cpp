//
//  FileLogger.cpp
//  interface/src
//
//  Created by Stojce Slavkovski on 12/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileLogger.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QHostAddress>

#include "NumericalConstants.h"
#include "FileUtils.h"
#include "SharedUtil.h"

static const QString FILENAME_FORMAT = "hifi-log_%1_%2.txt";
static const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
static const QString LOGS_DIRECTORY = "Logs";
// Max log size is 512 KB. We send log files to our crash reporter, so we want to keep this relatively
// small so it doesn't go over the 2MB zipped limit for all of the files we send.
static const qint64 MAX_LOG_SIZE = 512 * 1024;
// Max log age is 1 hour
static const uint64_t MAX_LOG_AGE_USECS = USECS_PER_SECOND * 3600;

static FilePersistThread* _persistThreadInstance;

QString getLogRollerFilename() {
    QString result = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress; // = getLocalAddress();
    QDateTime now = QDateTime::currentDateTime();
    result.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
    return result;
}

const QString& getLogFilename() {
    static QString fileName = FileUtils::standardPath(LOGS_DIRECTORY) + "hifi-log.txt";
    return fileName;
}

FilePersistThread::FilePersistThread(const FileLogger& logger) : _logger(logger) {
    setObjectName("LogFileWriter");

    // A file may exist from a previous run - if it does, roll the file and suppress notifying listeners.
    QFile file(_logger._fileName);
    if (file.exists()) {
        rollFileIfNecessary(file, false);
    }
    _lastRollTime = usecTimestampNow();
}

void FilePersistThread::rollFileIfNecessary(QFile& file, bool notifyListenersIfRolled) {
    uint64_t now = usecTimestampNow();
    if ((file.size() > MAX_LOG_SIZE) || (now - _lastRollTime) > MAX_LOG_AGE_USECS) {
        QString newFileName = getLogRollerFilename();
        if (file.copy(newFileName)) {
            file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            file.close();
            qDebug() << "Rolled log file:" << newFileName;

            if (notifyListenersIfRolled) {
                emit rollingLogFile(newFileName);
            }

            _lastRollTime = now;
        }
    }
}

bool FilePersistThread::processQueueItems(const Queue& messages) {
    QFile file(_logger._fileName);
    rollFileIfNecessary(file);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        foreach(const QString& message, messages) {
            out << message;
        }
    }
    return true;
}

FileLogger::FileLogger(QObject* parent) :
    QObject(parent), _fileName(getLogFilename())
{
    _persistThreadInstance = new FilePersistThread(*this);
    _persistThreadInstance->initialize(true, QThread::LowestPriority);
    connect(_persistThreadInstance, &FilePersistThread::rollingLogFile, this, &FileLogger::rollingLogFile);
}

FileLogger::~FileLogger() {
    _persistThreadInstance->terminate();
}

void FileLogger::addMessage(const QString& message) {
    _persistThreadInstance->queueItem(message);
    //emit logReceived(message);
}

void FileLogger::locateLog() {
    FileUtils::locateFile(_fileName);
}

QString FileLogger::getLogData() {
    QString result;
    QFile f(_fileName);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        result = QTextStream(&f).readAll();
    }
    return result;
}
