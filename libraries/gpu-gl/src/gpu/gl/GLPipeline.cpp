//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLPipeline.h"

#include "GLShader.h"
#include "GLState.h"

using namespace gpu;
using namespace gpu::gl;

GLPipeline* GLPipeline::sync(const Pipeline& pipeline) {
    GLPipeline* object = Backend::getGPUObject<GLPipeline>(pipeline);

    // If GPU object already created then good
    if (object) {
        return object;
    }

    // No object allocated yet, let's see if it's worth it...
    ShaderPointer shader = pipeline.getProgram();
    GLShader* programObject = GLShader::sync(*shader);
    if (programObject == nullptr) {
        return nullptr;
    }

    StatePointer state = pipeline.getState();
    GLState* stateObject = GLState::sync(*state);
    if (stateObject == nullptr) {
        return nullptr;
    }

    // Program and state are valid, we can create the pipeline object
    if (!object) {
        object = new GLPipeline();
        Backend::setGPUObject(pipeline, object);
    }

    object->_program = programObject;
    object->_state = stateObject;

    return object;
}
