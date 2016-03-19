//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DebugVrDisplayPlugin.h"


DebugVrDisplayPlugin::DebugVrDisplayPlugin() {
    _eyeProjections[0][0] = vec4(0.929788947, 0, 0, 0);
    _eyeProjections[0][1] = vec4(0, 0.750974476, 0, 0);
    _eyeProjections[0][2] = vec4(0.0156717598, 0, -1.00000489, -1);
    _eyeProjections[0][3] = vec4(0, 0, -0.0800003856, 0);
        
    _eyeProjections[1][0] = vec4(0.929788947, 0, 0, 0);
    _eyeProjections[1][1] = vec4(0, 0.750974476, 0, 0);
    _eyeProjections[1][2] = vec4(-0.0156717598, 0, -1.00000489, -1);
    _eyeProjections[1][3] = vec4(0, 0, -0.0800003856, 0);

    _eyeProjections[2][0] = vec4(0.929788947, 0, 0, 0);
    _eyeProjections[2][1] = vec4(0, 0.750974476, 0, 0);
    _eyeProjections[2][2] = vec4(0, 0, -1.00000489, -1);
    _eyeProjections[2][3] = vec4(0, 0, -0.0800003856, 0);

    _renderTargetSize = uvec2(2364, 1464);
    _eyeOffsets[0] = glm::translate(mat4(), vec3(-0.0306320004, 0, -0.000601001084));
    _eyeOffsets[1] = glm::translate(mat4(), vec3(0.0306320004, 0, -0.000601001084));
}

bool DebugVrDisplayPlugin::isSupported() const {
#ifdef DEBUG
    return true;
#else
    return false;
#endif
}

static const QString NAME("DebugVR");
const QString& DebugVrDisplayPlugin::getName() const {
    return NAME;
}
