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

using namespace shadertoy;

QHash<QString, Input> cachedTextures;

void initTextureCache() {
    using namespace shadertoy;

    for (int i = 0; i < TEXTURES.size(); ++i) {
        QString path = TEXTURES.at(i);
        qDebug() << "Loading texture from " << path;

        Input result;
        result.texture = load2dTexture(":" + path, result.size);

        result.resolution = vec3(result.size, 0);
        result.target = oglplus::TextureTarget::_2D;
        cachedTextures[path] = result;
    }

    for (int i = 0; i < CUBEMAPS.size(); ++i) {
        QString pathTemplate = CUBEMAPS.at(i);
        QString path = pathTemplate.arg(0);
        qDebug() << "Processing path " << path;
        Input result;
        result.texture = loadCubemapTexture([&](int i) {
            QString texturePath = pathTemplate.arg(i);
            ImagePtr image = loadImage(":" + texturePath, false);
            result.size = uvec2(image->Width(), image->Height());
            return image;
        });
        result.target = oglplus::TextureTarget::CubeMap;
        result.resolution = vec3(result.size, result.size.x);
        cachedTextures[path] = result;
    }
}

InputType typeForString(const QString& typeName) {
    if (typeName == "texture") {
        return InputType::TEXTURE;
    } else if (typeName == "buffer") {
        return InputType::BUFFER;
    } else if (typeName == "cubemap") {
        return InputType::CUBEMAP;
    } else if (typeName == "music") {
        return InputType::AUDIO;
    } else if (typeName == "video") {
        return InputType::VIDEO;
    } else if (typeName == "webcam") {
        return InputType::WEBCAM;
    } else if (typeName == "musicstream") {
        return InputType::NONE;
    } else if (typeName == "mic") {
        return InputType::NONE;
    } 
    return InputType::NONE;
}


//{
//    "id": 257,
//    "src" : "\/presets\/previz\/buffer00.png",
//    "ctype" : "buffer",
//    "channel" : 0,
//    "sampler" :
//    {
//        "filter": "linear",
//            "wrap" : "clamp",
//            "vflip" : "true",
//            "srgb" : "false",
//            "internal" : "byte"
//    }
//},
Input::Pointer getTexture(const QVariantMap& input) {
    const QString path = input["src"].toString();
    qDebug() << path;
    if (!cachedTextures.contains(path)) {
        return Input::Pointer();
    }

    Input result = cachedTextures[path];
    result.type = typeForString(input["ctype"].toString());
    //source = vmap["src"].toString();
    result.channel = input["channel"].isValid() ? input["channel"].toInt() : -1;

    //    "sampler" :
    //    {
    //        "filter": "linear",
    //            "wrap" : "clamp",
    //            "vflip" : "true",
    //            "srgb" : "false",
    //            "internal" : "byte"
    //    }
    auto sampler = input["sampler"].toMap();
    auto filter = sampler["filter"].toString();
    if (filter == "nearest") {
        result.sampler.minFilter = oglplus::TextureMinFilter::Nearest;
        result.sampler.magFilter = oglplus::TextureMagFilter::Nearest;
    } else if (filter == "linear") {
        result.sampler.minFilter = oglplus::TextureMinFilter::Linear;
        result.sampler.magFilter = oglplus::TextureMagFilter::Linear;
    } else if (filter == "mipmap") {
        result.sampler.minFilter = oglplus::TextureMinFilter::LinearMipmapLinear;
        result.sampler.magFilter = oglplus::TextureMagFilter::Linear;
    } else {
        result.sampler.minFilter = oglplus::TextureMinFilter::Nearest;
        result.sampler.magFilter = oglplus::TextureMagFilter::Nearest;
        qDebug() << "Warning, unknown filter type " << filter;
    }

    auto wrap = sampler["wrap"].toString();
    if (wrap == "clamp") {
        result.sampler.wrap = oglplus::TextureWrap::ClampToEdge;
    } else if (wrap == "repeat") {
        result.sampler.wrap = oglplus::TextureWrap::Repeat;
    }


    return std::make_shared<Input>(result);
}


void Input::bind() {
    if (texture && channel != -1) {
        using namespace oglplus;
        Texture::Active(channel);
        texture->Bind(target);
        Texture::WrapS(target, sampler.wrap);
        Texture::WrapT(target, sampler.wrap);
        Texture::WrapR(target, sampler.wrap);
        Texture::MinFilter(target, sampler.minFilter);
        Texture::MagFilter(target, sampler.magFilter);
    }
}