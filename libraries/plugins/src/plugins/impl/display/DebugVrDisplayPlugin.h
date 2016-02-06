//
//  Created by Bradley Austin Davis on 2016/02/07
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "WindowOpenGLDisplayPlugin.h"

class DebugVrDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    DebugVrDisplayPlugin();
    virtual bool isSupported() const override;

    virtual const QString & getName() const override;

    // Stereo specific methods
    virtual bool isHmd() const override final { return true; }
    virtual mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual uvec2 getRecommendedRenderSize() const override final;
    virtual uvec2 getRecommendedUiSize() const override final { return uvec2(1920, 1080); }
    virtual mat4 getHeadPose(uint32_t frameIndex) const override;

protected:
    uvec2 _desiredFramebufferSize;
    mat4 _eyeProjections[2];
    vec3 _eyeOffsets[2];
};
