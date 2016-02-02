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

#include <gl/OglplusHelpers.h>
#include <GLMHelpers.h>

#include "Shadertoy.h"

namespace shadertoy {

    struct Input {
        using Pointer = std::shared_ptr<Input>;

        QString source;
        InputType type{ NONE };
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

        virtual TexturePtr get() = 0;

        static Pointer load(const QVariant& v);

    private:
        Input();
    };
}