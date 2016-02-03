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

struct Input {
    using Pointer = std::shared_ptr<Input>;

    QString source;
    InputType type{ InputType::NONE };
    int channel{ -1 };

    struct {
        oglplus::TextureFilter filter{ oglplus::TextureFilter::Linear };
        oglplus::TextureWrap wrap{ oglplus::TextureWrap::ClampToEdge };
        bool flip{ true };
        bool srgb{ false };
        oglplus::PixelDataType format{ oglplus::PixelDataType::Byte };
    } sampler;

    vec3 resolution;
    uvec2 size;

    static Pointer load(const QVariant& v);

protected:
    Input() {}
};

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

struct BaseInput : public Input {
    BaseInput(const QVariantMap& vmap) {
        source = vmap["src"].toString();
        channel = vmap["channel"].isValid() ? vmap["channel"].toInt() : -1;
        auto sampler = vmap["sampler"].toMap();
        //filter = 
        //oglplus::TextureFilter filter{ oglplus::TextureFilter::Linear };
        //oglplus::TextureWrap wrap{ oglplus::TextureWrap::ClampToEdge };
        //bool flip{ true };
        //bool srgb{ false };
        //oglplus::PixelDataType format{ oglplus::PixelDataType::Byte };

    }

    TexturePtr get() {
        return TexturePtr();
    }
};

struct TextureInput : public BaseInput {
    TextureInput(const QVariantMap& vmap) : BaseInput(vmap) {
        type = InputType::TEXTURE;
    }
};

struct CubemapInput : public BaseInput {
    CubemapInput(const QVariantMap& vmap) : BaseInput(vmap) {
        type = InputType::TEXTURE;
    }

};

Input::Pointer Input::load(const QVariant& v) {
    auto vmap = v.toMap();
    auto ctype = typeForString(vmap["ctype"].toString());
    switch (ctype) {
    case InputType::TEXTURE:
        return std::make_shared<TextureInput>(vmap);
    case InputType::CUBEMAP:
        return std::make_shared<CubemapInput>(vmap);
    default:
        break;
    }
    return Input::Pointer();
}
