//
//  main.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QTranslator>
#include <QtCore/QElapsedTimer>
#include <SharedUtil.h>

#include "Application.h"

int main(int argc, const char* argv[]) {
    QElapsedTimer startupTime;
    startupTime.start();
    
    // Debug option to demonstrate that the client's local time does not 
    // need to be in sync with any other network node. This forces clock 
    // skew for the individual client
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(argc, argv, CLOCK_SKEW);
    if (clockSkewOption) {
        int clockSkew = atoi(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qDebug("clockSkewOption=%s clockSkew=%d", clockSkewOption, clockSkew);
    }
    int exitCode;
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        Application app(argc, const_cast<char**>(argv));

        QTranslator translator;
        translator.load("i18n/interface_en");
        app.installTranslator(&translator);
    
        qDebug("Created QT Application.");
        exitCode = app.exec();
    }

    qDebug("Normal exit.");
    return exitCode;
}   
