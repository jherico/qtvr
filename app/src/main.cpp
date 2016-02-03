//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <QtCore/QDebug>
#include <QtCore/QSettings>

int main(int argc, const char* argv[]) {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    int exitCode = Application(argc, const_cast<char**>(argv)).exec();
    qDebug("Normal exit.");
    return exitCode;
}   

