//
//  FileLogger.h
//  interface/src
//
//  Created by Stojce Slavkovski on 12/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileLogger_h
#define hifi_FileLogger_h

#include "GenericQueueThread.h"

#include <QtCore/QFile>

class FileLogger : public QObject {
    Q_OBJECT

public:
    FileLogger(QObject* parent = NULL);
    virtual ~FileLogger();

    QString getFilename() { return _fileName; }
    void addMessage(const QString&);
    QString getLogData();
    void locateLog();

signals:
    void rollingLogFile(QString newFilename);

private:
    const QString _fileName;
    friend class FilePersistThread;
};

class FilePersistThread : public GenericQueueThread < QString > {
    Q_OBJECT
public:
    FilePersistThread(const FileLogger& logger);

signals:
    void rollingLogFile(QString newFilename);

protected:
    void rollFileIfNecessary(QFile& file, bool notifyListenersIfRolled = true);
    virtual bool processQueueItems(const Queue& messages);

private:
    const FileLogger& _logger;
    uint64_t _lastRollTime;
};




#endif // hifi_FileLogger_h
