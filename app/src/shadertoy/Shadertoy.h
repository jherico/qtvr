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

#include <QtCore/QStringList>
#include <QtCore/QObject>

class Shadertoy : public QObject{
    Q_OBJECT
    Q_ENUMS(InputType)
    Q_ENUMS(Filter)
    Q_ENUMS(Wrap)
public:
    enum InputType { NONE, TEXTURE, CUBEMAP, VIDEO, AUDIO, WEBCAM, SOUNDCLOUD, KEYBOARD, BUFFER };
    enum Filter { NEAREST, LINEAR, MIPMAP };
    enum Wrap { CLAMP, REPEAT };

    static const int MAX_CHANNELS = 4;
    static const char* const UNIFORM_CHANNELS[MAX_CHANNELS];
    static const QRegularExpression VR_MARKER;
};

