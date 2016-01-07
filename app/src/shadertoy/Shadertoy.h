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

namespace shadertoy {
  static const int MAX_CHANNELS = 4;

  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

  extern const char* const UNIFORM_RESOLUTION;
  extern const char* const UNIFORM_GLOBALTIME;
  extern const char* const UNIFORM_CHANNEL_TIME;
  extern const char* const UNIFORM_CHANNEL_RESOLUTIONS[MAX_CHANNELS];
  extern const char* const UNIFORM_CHANNEL_RESOLUTION;
  extern const char* const UNIFORM_MOUSE_COORDS;
  extern const char* const UNIFORM_DATE;
  extern const char* const UNIFORM_SAMPLE_RATE;
  extern const char* const UNIFORM_POSITION;
  extern const char* const UNIFORM_CHANNELS[MAX_CHANNELS];

  extern const char* const SHADER_HEADER;
  extern const char* const LINE_NUMBER_HEADER;

  extern const QStringList TEXTURES;
  extern const QStringList CUBEMAPS;

  struct Shader {
    QString id;
    QString url;
    QString name;
    QString fragmentSource;
    bool vrEnabled{ false };
    ChannelInputType channelTypes[MAX_CHANNELS];
    QString channelTextures[MAX_CHANNELS];
  };

  Shader loadShaderFile(const QString & shaderPath);
  void saveShaderXml(const QString & shaderPath, const Shader & shader);
}
