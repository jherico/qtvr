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

#include "Shadertoy.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QVariant>

#include <QtXml/QDomDocument>

inline QByteArray readFileToByteArray(const QString & fileName) {
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

QJsonValue path(const QJsonValue & parent, std::initializer_list<QVariant> elements) {
    QJsonValue current = parent;
    std::for_each(elements.begin(), elements.end(), [&](const QVariant & element) {
        if (current.isObject()) {
            QString path = element.toString();
            current = current.toObject().value(path);
        } else if (current.isArray()) {
            int offset = element.toInt();
            current = current.toArray().at(offset);
        } else {
            qWarning() << "Unable to continue";
            current = QJsonValue();
        }
    });
    return current;
}


namespace shadertoy {

    const char* const UNIFORM_RESOLUTION = "iResolution";
    const char* const UNIFORM_GLOBALTIME = "iGlobalTime";
    const char* const UNIFORM_CHANNEL_TIME = "iChannelTime";
    const char* const UNIFORM_CHANNEL_RESOLUTIONS[MAX_CHANNELS] = {
        "iChannelResolution[0]",
        "iChannelResolution[1]",
        "iChannelResolution[2]",
        "iChannelResolution[3]",
    };

    const char* const UNIFORM_CHANNEL_RESOLUTION = "iChannelResolution";
    const char* const UNIFORM_MOUSE_COORDS = "iMouse";
    const char* const UNIFORM_DATE = "iDate";
    const char* const UNIFORM_SAMPLE_RATE = "iSampleRate";
    const char* const UNIFORM_POSITION = "iPos";
    const char* const UNIFORM_CHANNELS[MAX_CHANNELS] = {
        "iChannel0",
        "iChannel1",
        "iChannel2",
        "iChannel3",
    };

    const char* const SHADER_HEADER = "#version 330\n"
        "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
        "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
        "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
        "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
        "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
        "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
        "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
        "uniform vec3      iPos; // Head position\n"
        "in vec3 iDir; // Direction from viewer\n"
        "out vec4 FragColor;\n";

    const char* const LINE_NUMBER_HEADER =
        "#line 1\n";

    const QStringList TEXTURES({
        "/presets/tex00.jpg",
        "/presets/tex01.jpg",
        "/presets/tex02.jpg",
        "/presets/tex03.jpg",
        "/presets/tex04.jpg",
        "/presets/tex05.jpg",
        "/presets/tex06.jpg",
        "/presets/tex07.jpg",
        "/presets/tex08.jpg",
        "/presets/tex09.jpg",
        "/presets/tex10.png",
        "/presets/tex11.png",
        "/presets/tex12.png",
        "/presets/tex14.png",
        "/presets/tex15.png",
        "/presets/tex16.png"
    });

    const QStringList CUBEMAPS({
        "/presets/cube00_%1.jpg",
        "/presets/cube01_%1.png",
        "/presets/cube02_%1.jpg",
        "/presets/cube03_%1.png",
        "/presets/cube04_%1.png",
        "/presets/cube05_%1.png",
    });

    static const char* const CHANNEL_REGEX = "(\\w+)(\\d{2})";
    static const char* const XML_ROOT_NAME = "shadertoy";
    static const char* const XML_FRAGMENT_SOURCE = "fragmentSource";
    static const char* const XML_NAME = "name";
    static const char* const XML_CHANNEL = "channel";
    static const char* const XML_CHANNEL_ATTR_ID = "id";
    static const char* const XML_CHANNEL_ATTR_SOURCE = "source";
    static const char* const XML_CHANNEL_ATTR_TYPE = "type";

    ChannelInputType channelTypeFromString(const QString & channelTypeStr) {
        ChannelInputType channelType = ChannelInputType::TEXTURE;
        if (channelTypeStr == "tex") {
            channelType = ChannelInputType::TEXTURE;
        } else if (channelTypeStr == "cube") {
            channelType = ChannelInputType::CUBEMAP;
        }
        return channelType;
    }


    ChannelInputType fromShadertoyString(const QString & channelType) {
        // texture music cubemap ???
        if (channelType == "cubemap") {
            return ChannelInputType::CUBEMAP;
        } else if (channelType == "texture") {
            return ChannelInputType::TEXTURE;
        } else if (channelType == "music") {
            return ChannelInputType::AUDIO;
        } else {
            // FIXME add support for video
            throw std::exception("Unable to parse channel type");
        }
    }

    Shader parseShaderJson(const QByteArray & shaderJson) {
        Shader result;
        QJsonDocument jsonResponse = QJsonDocument::fromJson(shaderJson);
        QJsonObject jsonObject = jsonResponse.object();
        QJsonObject info = path(jsonResponse.object(), { "Shader", 0, "info" }).toObject();
        result.name = info["name"].toString().toLocal8Bit();
        result.id = info["id"].toString().toLocal8Bit();
        //result.description = info["description"].toString().toLocal8Bit();
        QJsonObject renderPass = path(jsonResponse.object(), { "Shader", 0, "renderpass", 0 }).toObject();
        result.fragmentSource = renderPass["code"].toString().toLocal8Bit();
        QJsonArray inputs = renderPass["inputs"].toArray();
        for (int i = 0; i < inputs.count(); ++i) {
            QJsonObject channel = inputs.at(i).toObject();
            int channelIndex = channel["channel"].toInt();
            result.channelTypes[channelIndex] = fromShadertoyString(channel["ctype"].toString());
            result.channelTextures[channelIndex] = channel["src"].toString().toLocal8Bit();
        }
        result.vrEnabled = result.fragmentSource.contains("#pragma vr");
        return result;
    }

    Shader loadShaderXml(QIODevice & ioDevice) {
        QDomDocument dom;
        Shader result;
        dom.setContent(&ioDevice);

        auto children = dom.documentElement().childNodes();
        for (int i = 0; i < children.count(); ++i) {
            auto child = children.at(i);
            if (child.nodeName() == "url") {
                result.url = child.firstChild().nodeValue();
            } if (child.nodeName() == XML_FRAGMENT_SOURCE) {
                result.fragmentSource = child.firstChild().nodeValue();
            } if (child.nodeName() == XML_NAME) {
                result.name = child.firstChild().nodeValue();
            } else if (child.nodeName() == XML_CHANNEL) {
                auto attributes = child.attributes();
                int channelIndex = -1;
                QString source;
                if (attributes.contains(XML_CHANNEL_ATTR_ID)) {
                    channelIndex = attributes.namedItem(XML_CHANNEL_ATTR_ID).nodeValue().toInt();
                }

                if (channelIndex < 0 || channelIndex >= shadertoy::MAX_CHANNELS) {
                    continue;
                }


                // Compatibility mode
                if (attributes.contains(XML_CHANNEL_ATTR_SOURCE)) {
                    source = attributes.namedItem(XML_CHANNEL_ATTR_SOURCE).nodeValue();
                    QRegExp re(CHANNEL_REGEX);
                    if (!re.exactMatch(source)) {
                        continue;
                    }
                    result.channelTypes[channelIndex] = channelTypeFromString(re.cap(1));
                    result.channelTextures[channelIndex] = ("preset://" + re.cap(1) + "/" + re.cap(2));
                    continue;
                }

                if (attributes.contains(XML_CHANNEL_ATTR_TYPE)) {
                    result.channelTypes[channelIndex] = channelTypeFromString(attributes.namedItem(XML_CHANNEL_ATTR_SOURCE).nodeValue());
                    result.channelTextures[channelIndex] = child.firstChild().nodeValue();
                }
            }
        }
        result.vrEnabled = result.fragmentSource.contains("#pragma vr");
        return result;
    }

    Shader loadShaderJson(const QString & shaderPath) {
        QByteArray json = readFileToByteArray(shaderPath);
        return parseShaderJson(json);
    }

    Shader loadShaderXml(const QString & fileName) {
        QFile file(fileName);
        return loadShaderXml(file);
    }

    Shader loadShaderFile(const QString & shaderPath) {
        if (shaderPath.endsWith(".xml", Qt::CaseInsensitive)) {
            return shadertoy::loadShaderXml(shaderPath);
        } else if (shaderPath.endsWith(".json", Qt::CaseInsensitive)) {
            return shadertoy::loadShaderJson(shaderPath);
        } else {
            qWarning() << "Don't know how to parse path " << shaderPath;
        }
        return Shader();
    }

    QByteArray readFile(const QString & filename) {
        QFile file(filename);
        file.open(QFile::ReadOnly);
        QByteArray result = file.readAll();
        file.close();
        return result;
    }

    // FIXME no error handling.
    QDomDocument writeShaderXml(const Shader & shader) {
        QDomDocument result;
        QDomElement root = result.createElement(XML_ROOT_NAME);
        result.appendChild(root);

        for (int i = 0; i < MAX_CHANNELS; ++i) {
            if (!shader.channelTextures[i].isEmpty()) {
                QDomElement channelElement = result.createElement(XML_CHANNEL);
                channelElement.setAttribute(XML_CHANNEL_ATTR_ID, i);
                channelElement.setAttribute(XML_CHANNEL_ATTR_TYPE, shader.channelTypes[i] == ChannelInputType::CUBEMAP ? "cube" : "tex");
                channelElement.appendChild(result.createTextNode(shader.channelTextures[i]));
                root.appendChild(channelElement);
            }
        }
        root.appendChild(result.createElement(XML_FRAGMENT_SOURCE)).
            appendChild(result.createCDATASection(shader.fragmentSource));
        if (!shader.name.isEmpty()) {
            root.appendChild(result.createElement(XML_NAME)).
                appendChild(result.createCDATASection(shader.name));
        }
        return result;
    }


    // FIXME no error handling.
    void saveShaderXml(const QString & name, const Shader & shader) {
        QDomDocument doc = writeShaderXml(shader);
        QFile file(name);
        file.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text);
        file.write(doc.toByteArray());
        file.close();
    }
}
