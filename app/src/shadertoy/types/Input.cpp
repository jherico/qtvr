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

#include "Input.h"
#include <QtCore/QVariant>

Input::Type Input::toType(const QVariant& value) {
    auto str = value.toString();
    if (str == "texture") {
        return Input::TEXTURE;
    } else if (str == "cubemap") {
        return Input::CUBEMAP;
    } else if (str == "video") {
        return Input::VIDEO;
    } else if (str == "music") {
        return Input::AUDIO;
    } else if (str == "webcam") {
        return Input::WEBCAM;
    } else if (str == "musicstream") {
        return Input::SOUNDCLOUD;
    } else if (str == "keyboard") {
        return Input::KEYBOARD;
    } else if (str == "mic") {
        return Input::MIC;
    } else if (str == "buffer") {
        return Input::BUFFER;
    } 
      
    return Input::NONE;
}

Input::Filter Input::toFilter(const QVariant& value) {
    auto str = value.toString();
    if (str == "nearest") {
        return Input::NEAREST;
    } else if (str == "linear") {
        return Input::LINEAR;
    } else if (str == "mipmap") {
        return Input::MIPMAP;
    }
    return Input::NEAREST;
}

Input::Wrap Input::toWrap(const QVariant& value) {
    auto str = value.toString();
    if (str == "clamp") {
        return Input::CLAMP;
    } else if (str == "repeat") {
        return Input::REPEAT;
    }
    return Input::CLAMP;

}

bool Input::parse(const QVariant& var) {
    auto varMap = var.toMap();
    src = varMap["src"].toString();
    channel = varMap["channel"].toInt();
    ctype = toType(varMap["ctype"]);
    auto sampler = varMap["sampler"].toMap();;
    wrap = toWrap(sampler["wrap"]);
    filter = toFilter(sampler["filter"]);
    vflip = sampler["vflip"].toBool();
    srgb = sampler["srgb"].toBool();
    return true;
}

//Input::Pointer getTexture(const QVariantMap& input) {
//    const QString path = input["src"].toString();
//    qDebug() << path;
//    if (!cachedTextures.contains(path)) {
//        return Input::Pointer();
//    }
//
//    Input result = cachedTextures[path];
//    result.type = typeForString(input["ctype"].toString());
//    //source = vmap["src"].toString();
//    result.channel = input["channel"].isValid() ? input["channel"].toInt() : -1;
//
//    //    "sampler" :
//    //    {
//    //        "filter": "linear",
//    //            "wrap" : "clamp",
//    //            "vflip" : "true",
//    //            "srgb" : "false",
//    //            "internal" : "byte"
//    //    }
//    auto sampler = input["sampler"].toMap();
//    auto filter = sampler["filter"].toString();
//    if (filter == "nearest") {
//        result.sampler.minFilter = oglplus::TextureMinFilter::Nearest;
//        result.sampler.magFilter = oglplus::TextureMagFilter::Nearest;
//    } else if (filter == "linear") {
//        result.sampler.minFilter = oglplus::TextureMinFilter::Linear;
//        result.sampler.magFilter = oglplus::TextureMagFilter::Linear;
//    } else if (filter == "mipmap") {
//        result.sampler.minFilter = oglplus::TextureMinFilter::LinearMipmapLinear;
//        result.sampler.magFilter = oglplus::TextureMagFilter::Linear;
//    } else {
//        result.sampler.minFilter = oglplus::TextureMinFilter::Nearest;
//        result.sampler.magFilter = oglplus::TextureMagFilter::Nearest;
//        qDebug() << "Warning, unknown filter type " << filter;
//    }
//
//    auto wrap = sampler["wrap"].toString();
//    if (wrap == "clamp") {
//        result.sampler.wrap = oglplus::TextureWrap::ClampToEdge;
//    } else if (wrap == "repeat") {
//        result.sampler.wrap = oglplus::TextureWrap::Repeat;
//    }
//
//
//    return std::make_shared<Input>(result);
//}
//
//
//void Input::bind(int channel) {
//}