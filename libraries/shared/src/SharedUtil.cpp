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

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QtCore/QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QThread>

#include "NumericalConstants.h"
#include "SharedLogging.h"
#include "SharedUtil.h"

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
