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

struct Input : public QObject {
public:
    enum Type { NONE, TEXTURE, CUBEMAP, VIDEO, AUDIO, WEBCAM, SOUNDCLOUD, MIC, KEYBOARD, BUFFER };
    enum Filter { NEAREST, LINEAR, MIPMAP };
    enum Wrap { CLAMP, REPEAT };

    Q_OBJECT
    Q_PROPERTY(QString src MEMBER src CONSTANT)
    Q_PROPERTY(int channel MEMBER channel CONSTANT)
    Q_PROPERTY(Type ctype MEMBER ctype CONSTANT)
    Q_PROPERTY(Wrap wrap MEMBER wrap CONSTANT)
    Q_PROPERTY(bool vflip MEMBER vflip CONSTANT)
    Q_PROPERTY(Filter filter MEMBER filter CONSTANT)
    Q_PROPERTY(bool srgb MEMBER srgb CONSTANT)

    Q_ENUMS(Type)
    Q_ENUMS(Filter)
    Q_ENUMS(Wrap)

    
public:
    static Type toType(const QVariant& value);
    static Filter toFilter(const QVariant& value);
    static Wrap toWrap(const QVariant& value);

    Input(QObject* parent = nullptr) : QObject(parent) {}
    QString src;
    int channel{ -1 };
    Type ctype{ TEXTURE };
    Wrap wrap{ CLAMP };
    bool vflip{ true };
    Filter filter{ NEAREST };
    bool srgb{ false };

    bool parse(const QVariant& renderpass);
};
