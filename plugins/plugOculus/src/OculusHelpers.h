//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <OVR_CAPI_GL.h>
#include <GLMHelpers.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if (OVR_MAJOR_VERSION < 6)
#define OVR_SUCCESS(x) x
#endif

// Convenience method for looping over each eye with a lambda
template <typename Function>
inline void ovr_for_each_eye(Function function) {
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) {
        function(eye);
    }
}

inline glm::mat4 toGlm(const ovrMatrix4f & om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
}

inline glm::mat4 toGlm(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
    return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
}

inline glm::vec3 toGlm(const ovrVector3f & ov) {
    return glm::make_vec3(&ov.x);
}

inline glm::vec2 toGlm(const ovrVector2f & ov) {
    return glm::make_vec2(&ov.x);
}

inline glm::uvec2 toGlm(const ovrSizei & ov) {
    return glm::uvec2(ov.w, ov.h);
}

inline glm::quat toGlm(const ovrQuatf & oq) {
    return glm::make_quat(&oq.x);
}

inline glm::mat4 toGlm(const ovrPosef & op) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(), toGlm(op.Position));
    return translation * orientation;
}

inline ovrMatrix4f ovrFromGlm(const glm::mat4 & m) {
    ovrMatrix4f result;
    glm::mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
}

inline ovrVector3f ovrFromGlm(const glm::vec3 & v) {
    return{ v.x, v.y, v.z };
}

inline ovrVector2f ovrFromGlm(const glm::vec2 & v) {
    return{ v.x, v.y };
}

inline ovrSizei ovrFromGlm(const glm::uvec2 & v) {
    return{ (int)v.x, (int)v.y };
}

inline ovrQuatf ovrFromGlm(const glm::quat & q) {
    return{ q.x, q.y, q.z, q.w };
}

inline ovrPosef ovrPoseFromGlm(const glm::mat4 & m) {
    glm::vec3 translation = glm::vec3(m[3]) / m[3].w;
    glm::quat orientation = glm::quat_cast(m);
    ovrPosef result;
    result.Orientation = ovrFromGlm(orientation);
    result.Position = ovrFromGlm(translation);
    return result; 
}
