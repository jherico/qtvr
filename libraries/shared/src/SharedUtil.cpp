//
//  SharedUtil.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedUtil.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <time.h>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef Q_OS_WIN
#include "CPUIdent.h"
#endif


#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QtCore/QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QProcess>
#include <QSysInfo>
#include <QThread>

#include "NumericalConstants.h"
#include "SharedLogging.h"

static int usecTimestampNowAdjust = 0; // in usec
void usecTimestampNowForceClockSkew(int clockSkew) {
    ::usecTimestampNowAdjust = clockSkew;
}

static qint64 TIME_REFERENCE = 0; // in usec
static std::once_flag usecTimestampNowIsInitialized;
static QElapsedTimer timestampTimer;

quint64 usecTimestampNow(bool wantDebug) {
    std::call_once(usecTimestampNowIsInitialized, [&] {
        TIME_REFERENCE = QDateTime::currentMSecsSinceEpoch() * USECS_PER_MSEC; // ms to usec
        timestampTimer.start();
    });
    
    quint64 now;
    quint64 nsecsElapsed = timestampTimer.nsecsElapsed();
    quint64 usecsElapsed = nsecsElapsed / NSECS_PER_USEC;  // nsec to usec
    
    // QElapsedTimer may not advance if the CPU has gone to sleep. In which case it
    // will begin to deviate from real time. We detect that here, and reset if necessary
    quint64 msecsCurrentTime = QDateTime::currentMSecsSinceEpoch();
    quint64 msecsEstimate = (TIME_REFERENCE + usecsElapsed) / USECS_PER_MSEC; // usecs to msecs
    int possibleSkew = msecsEstimate - msecsCurrentTime;
    const int TOLERANCE = 10 * MSECS_PER_SECOND; // up to 10 seconds of skew is tolerated
    if (abs(possibleSkew) > TOLERANCE) {
        // reset our TIME_REFERENCE and timer
        TIME_REFERENCE = QDateTime::currentMSecsSinceEpoch() * USECS_PER_MSEC; // ms to usec
        timestampTimer.restart();
        now = TIME_REFERENCE + ::usecTimestampNowAdjust;

        if (wantDebug) {
            qCDebug(shared) << "usecTimestampNow() - resetting QElapsedTimer. ";
            qCDebug(shared) << "    msecsCurrentTime:" << msecsCurrentTime;
            qCDebug(shared) << "       msecsEstimate:" << msecsEstimate;
            qCDebug(shared) << "        possibleSkew:" << possibleSkew;
            qCDebug(shared) << "           TOLERANCE:" << TOLERANCE;
            
            qCDebug(shared) << "        nsecsElapsed:" << nsecsElapsed;
            qCDebug(shared) << "        usecsElapsed:" << usecsElapsed;

            QDateTime currentLocalTime = QDateTime::currentDateTime();

            quint64 msecsNow = now / 1000; // usecs to msecs
            QDateTime nowAsString;
            nowAsString.setMSecsSinceEpoch(msecsNow);

            qCDebug(shared) << "                 now:" << now;
            qCDebug(shared) << "            msecsNow:" << msecsNow;

            qCDebug(shared) << "         nowAsString:" << nowAsString.toString("yyyy-MM-dd hh:mm:ss.zzz");
            qCDebug(shared) << "    currentLocalTime:" << currentLocalTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
        }
    } else {
        now = TIME_REFERENCE + usecsElapsed + ::usecTimestampNowAdjust;
    }

    if (wantDebug) {
        QDateTime currentLocalTime = QDateTime::currentDateTime();

        quint64 msecsNow = now / 1000; // usecs to msecs
        QDateTime nowAsString;
        nowAsString.setMSecsSinceEpoch(msecsNow);

        quint64 msecsTimeReference = TIME_REFERENCE / 1000; // usecs to msecs
        QDateTime timeReferenceAsString;
        timeReferenceAsString.setMSecsSinceEpoch(msecsTimeReference);

        qCDebug(shared) << "usecTimestampNow() - details... ";
        qCDebug(shared) << "           TIME_REFERENCE:" << TIME_REFERENCE;
        qCDebug(shared) << "    timeReferenceAsString:" << timeReferenceAsString.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qCDebug(shared) << "   usecTimestampNowAdjust:" << usecTimestampNowAdjust;
        qCDebug(shared) << "             nsecsElapsed:" << nsecsElapsed;
        qCDebug(shared) << "             usecsElapsed:" << usecsElapsed;
        qCDebug(shared) << "                      now:" << now;
        qCDebug(shared) << "                 msecsNow:" << msecsNow;
        qCDebug(shared) << "              nowAsString:" << nowAsString.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qCDebug(shared) << "         currentLocalTime:" << currentLocalTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
    
    return now;
}

float secTimestampNow() {
    static const auto START_TIME = usecTimestampNow();
    const auto nowUsecs = usecTimestampNow() - START_TIME;
    const auto nowMsecs = nowUsecs / USECS_PER_MSEC;
    return (float)nowMsecs / MSECS_PER_SECOND;
}

float randFloat() {
    return (rand() % 10000)/10000.0f;
}

int randIntInRange (int min, int max) {
    return min + (rand() % ((max + 1) - min));
}

float randFloatInRange (float min,float max) {
    return min + ((rand() % 10000)/10000.0f * (max-min));
}

float randomSign() {
    return randomBoolean() ? -1.0 : 1.0;
}

unsigned char randomColorValue(int miniumum) {
    return miniumum + (rand() % (256 - miniumum));
}

bool randomBoolean() {
    return rand() % 2;
}

bool shouldDo(float desiredInterval, float deltaTime) {
    return randFloat() < deltaTime / desiredInterval;
}

bool isBetween(int64_t value, int64_t max, int64_t min) {
    return ((value <= max) && (value >= min));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line flag and option was
//              included while launching the application, and to get the option value
//              immediately following the flag. For example if you ran:
//                      ./app -i filename.txt
//              then you're using the "-i" flag to set the input file name.
// Usage:       char * inputFilename = getCmdOption(argc, argv, "-i");
// Complaints:  Brad :)
const char* getCmdOption(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0 && i+1 < argc) {
            // then return the next option
            return argv[i+1];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line option flag was
//              included while launching the application. Returns bool true/false
// Usage:       bool wantDump   = cmdOptionExists(argc, argv, "-d");
// Complaints:  Brad :)

bool cmdOptionExists(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0) {
            // then return the next option
            return true;
        }
    }
    return false;
}

void sharedMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message) {
    fprintf(stdout, "%s", message.toLocal8Bit().constData());
}

#ifdef _WIN32
    void usleep(int waitTime) {
        const quint64 BUSY_LOOP_USECS = 2000;
        quint64 compTime = waitTime + usecTimestampNow();
        quint64 compTimeSleep = compTime - BUSY_LOOP_USECS;
        while (true) {
            if (usecTimestampNow() < compTimeSleep) {
                QThread::msleep(1);
            }
            if (usecTimestampNow() >= compTime) {
                break;
            }
        }
    }
#endif

float SMALL_LIMIT = 10.0f;
float LARGE_LIMIT = 1000.0f;

int packFloatRatioToTwoByte(unsigned char* buffer, float ratio) {
    // if the ratio is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    int16_t ratioHolder;

    if (ratio < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        ratioHolder = floorf(ratio * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        const float LARGE_RATIO_CONVERSION_RATIO = std::numeric_limits<int16_t>::min() / LARGE_LIMIT;
        ratioHolder = floorf((std::min(ratio,LARGE_LIMIT) - SMALL_LIMIT) * LARGE_RATIO_CONVERSION_RATIO);
    }
    memcpy(buffer, &ratioHolder, sizeof(ratioHolder));
    return sizeof(ratioHolder);
}

int unpackFloatRatioFromTwoByte(const unsigned char* buffer, float& ratio) {
    int16_t ratioHolder;
    memcpy(&ratioHolder, buffer, sizeof(ratioHolder));

    // If it's positive, than the original ratio was less than SMALL_LIMIT
    if (ratioHolder > 0) {
        ratio = (ratioHolder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original ratio was between SMALL_LIMIT and LARGE_LIMIT
        ratio = ((ratioHolder / (float) std::numeric_limits<int16_t>::min()) * LARGE_LIMIT) + SMALL_LIMIT;
    }
    return sizeof(ratioHolder);
}

int packClipValueToTwoByte(unsigned char* buffer, float clipValue) {
    // Clip values must be less than max signed 16bit integers
    assert(clipValue < std::numeric_limits<int16_t>::max());
    int16_t holder;

    // if the clip is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    if (clipValue < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        holder = floorf(clipValue * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        // otherwise we store it as a negative integer
        holder = -1 * floorf(clipValue);
    }
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackClipValueFromTwoByte(const unsigned char* buffer, float& clipValue) {
    int16_t holder;
    memcpy(&holder, buffer, sizeof(holder));

    // If it's positive, than the original clipValue was less than SMALL_LIMIT
    if (holder > 0) {
        clipValue = (holder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original holder can be found as the opposite sign of holder
        clipValue = -1.0f * holder;
    }
    return sizeof(holder);
}

int packFloatToByte(unsigned char* buffer, float value, float scaleBy) {
    quint8 holder;
    const float CONVERSION_RATIO = (255 / scaleBy);
    holder = floorf(value * CONVERSION_RATIO);
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackFloatFromByte(const unsigned char* buffer, float& value, float scaleBy) {
    quint8 holder;
    memcpy(&holder, buffer, sizeof(holder));
    value = ((float)holder / (float) 255) * scaleBy;
    return sizeof(holder);
}

bool isNaN(float value) {
    return value != value;
}

QString formatUsecTime(float usecs, int prec) {
    static const quint64 SECONDS_PER_MINUTE = 60;
    static const quint64 USECS_PER_MINUTE = USECS_PER_SECOND * SECONDS_PER_MINUTE;

    QString result;
    if (usecs > USECS_PER_MINUTE) {
        result = QString::number(usecs / USECS_PER_MINUTE, 'f', prec) + "min";
    } else if (usecs > USECS_PER_SECOND) {
        result = QString::number(usecs / USECS_PER_SECOND, 'f', prec) + 's';
    } else if (usecs > USECS_PER_MSEC) {
        result = QString::number(usecs / USECS_PER_MSEC, 'f', prec) + "ms";
    } else {
        result = QString::number(usecs, 'f', prec) + "us";
    }
    return result;
}

QString formatSecondsElapsed(float seconds) {
    QString result;

    const float SECONDS_IN_DAY = 60.0f * 60.0f * 24.0f;        
    if (seconds > SECONDS_IN_DAY) {
        float days = floor(seconds / SECONDS_IN_DAY);
        float rest = seconds - (days * SECONDS_IN_DAY);
        result = QString::number((int)days);
        if (days > 1.0f) {
            result += " days ";
        } else {
            result += " day ";
        }
        result += QDateTime::fromTime_t(rest).toUTC().toString("h 'hours' m 'minutes' s 'seconds'");
    } else {
        result = QDateTime::fromTime_t(seconds).toUTC().toString("h 'hours' m 'minutes' s 'seconds'");
    }
    return result;
}

bool similarStrings(const QString& stringA, const QString& stringB) {
    QStringList aWords = stringA.split(" ");
    QStringList bWords = stringB.split(" ");
    float aWordsInB = 0.0f;
    foreach(QString aWord, aWords) {
        if (bWords.contains(aWord)) {
            aWordsInB += 1.0f;
        }
    }
    float bWordsInA = 0.0f;
    foreach(QString bWord, bWords) {
        if (aWords.contains(bWord)) {
            bWordsInA += 1.0f;
        }
    }
    float similarity = 0.5f * (aWordsInB / (float)bWords.size()) + 0.5f * (bWordsInA / (float)aWords.size());
    const float SIMILAR_ENOUGH = 0.5f; // half the words the same is similar enough for us
    return similarity >= SIMILAR_ENOUGH;
}

void disableQtBearerPoll() {
    // to work around the Qt constant wireless scanning, set the env for polling interval very high
    const QByteArray EXTREME_BEARER_POLL_TIMEOUT = QString::number(INT_MAX).toLocal8Bit();
    qputenv("QT_BEARER_POLL_TIMEOUT", EXTREME_BEARER_POLL_TIMEOUT);
}

void printSystemInformation() {
    // Write system information to log
    qDebug() << "Build Information";
    qDebug().noquote() << "\tBuild ABI: " << QSysInfo::buildAbi();
    qDebug().noquote() << "\tBuild CPU Architecture: " << QSysInfo::buildCpuArchitecture();

    qDebug().noquote() << "System Information";
    qDebug().noquote() << "\tProduct Name: " << QSysInfo::prettyProductName();
    qDebug().noquote() << "\tCPU Architecture: " << QSysInfo::currentCpuArchitecture();
    qDebug().noquote() << "\tKernel Type: " << QSysInfo::kernelType();
    qDebug().noquote() << "\tKernel Version: " << QSysInfo::kernelVersion();

    auto macVersion = QSysInfo::macVersion();
    if (macVersion != QSysInfo::MV_None) {
        qDebug() << "\tMac Version: " << macVersion;
    }

    auto windowsVersion = QSysInfo::windowsVersion();
    if (windowsVersion != QSysInfo::WV_None) {
        qDebug() << "\tWindows Version: " << windowsVersion;
    }

#ifdef Q_OS_WIN
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    qDebug() << "SYSTEM_INFO";
    qDebug().noquote() << "\tOEM ID: " << si.dwOemId;
    qDebug().noquote() << "\tProcessor Architecture: " << si.wProcessorArchitecture;
    qDebug().noquote() << "\tProcessor Type: " << si.dwProcessorType;
    qDebug().noquote() << "\tProcessor Level: " << si.wProcessorLevel;
    qDebug().noquote() << "\tProcessor Revision: "
                       << QString("0x%1").arg(si.wProcessorRevision, 4, 16, QChar('0'));
    qDebug().noquote() << "\tNumber of Processors: " << si.dwNumberOfProcessors;
    qDebug().noquote() << "\tPage size: " << si.dwPageSize << " Bytes";
    qDebug().noquote() << "\tMin Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMinimumApplicationAddress), 16, 16, QChar('0'));
    qDebug().noquote() << "\tMax Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMaximumApplicationAddress), 16, 16, QChar('0'));

    const double BYTES_TO_MEGABYTE = 1.0 / (1024 * 1024);

    qDebug() << "MEMORYSTATUSEX";
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        qDebug().noquote() << QString("\tCurrent System Memory Usage: %1%").arg(ms.dwMemoryLoad);
        qDebug().noquote() << QString("\tAvail Physical Memory: %1 MB").arg(ms.ullAvailPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qDebug().noquote() << QString("\tTotal Physical Memory: %1 MB").arg(ms.ullTotalPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qDebug().noquote() << QString("\tAvail in Page File:    %1 MB").arg(ms.ullAvailPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qDebug().noquote() << QString("\tTotal in Page File:    %1 MB").arg(ms.ullTotalPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qDebug().noquote() << QString("\tAvail Virtual Memory:  %1 MB").arg(ms.ullAvailVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qDebug().noquote() << QString("\tTotal Virtual Memory:  %1 MB").arg(ms.ullTotalVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
    } else {
        qDebug() << "\tFailed to retrieve memory status: " << GetLastError();
    }

    qDebug() << "CPUID";

    qDebug() << "\tCPU Vendor: " << CPUIdent::Vendor().c_str();
    qDebug() << "\tCPU Brand:  " << CPUIdent::Brand().c_str();

    for (auto& feature : CPUIdent::getAllFeatures()) {
        qDebug().nospace().noquote() << "\t[" << (feature.supported ? "x" : " ") << "] " << feature.name.c_str();
    }
#endif

    qDebug() << "Environment Variables";
    // List of env variables to include in the log. For privacy reasons we don't send all env variables.
    const QStringList envWhitelist = {
        "QTWEBENGINE_REMOTE_DEBUGGING"
    };
    auto envVariables = QProcessEnvironment::systemEnvironment();
    for (auto& env : envWhitelist)
    {
        qDebug().noquote().nospace() << "\t" <<
            (envVariables.contains(env) ? " = " + envVariables.value(env) : " NOT FOUND");
    }
}
