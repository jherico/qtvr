/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QQmlListProperty>

#include "Input.h"

struct Renderpass : public QObject {
public:
    enum Output { IMAGE, BUFFER_A, BUFFER_B, BUFFER_C, BUFFER_D, SOUND };

private:
    Q_OBJECT
    Q_PROPERTY(QString code MEMBER code CONSTANT)
    Q_PROPERTY(Output output MEMBER output CONSTANT)
    Q_PROPERTY(QQmlListProperty<Input> inputs READ inputs CONSTANT)
    Q_ENUMS(Output)
public:
    Renderpass(QObject* parent = nullptr) : QObject(parent) {}
    QString code;
    Output output { IMAGE };

    QQmlListProperty<Input> inputs() { return QQmlListProperty<Input>(this, _inputs); }
    bool parse(const QVariant& var); 

    QList<Input*> _inputs;
};