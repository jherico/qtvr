//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusBaseDisplayPlugin.h"

#include <NumericalConstants.h>

#include "OculusHelpers.h"

void OculusBaseDisplayPlugin::resetSensors() {
    ovr_RecenterTrackingOrigin(_session);
}

void OculusBaseDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    _currentRenderFrameInfo = FrameInfo();
    _currentRenderFrameInfo.sensorSampleTime = ovr_GetTimeInSeconds();;
    _currentRenderFrameInfo.predictedDisplayTime = ovr_GetPredictedDisplayTime(_session, frameIndex);
    auto trackingState = ovr_GetTrackingState(_session, _currentRenderFrameInfo.predictedDisplayTime, ovrTrue);
    _currentRenderFrameInfo.renderPose = toGlm(trackingState.HeadPose.ThePose);
    _currentRenderFrameInfo.presentPose = _currentRenderFrameInfo.renderPose;
    Lock lock(_mutex);
    _frameInfos[frameIndex] = _currentRenderFrameInfo;
}

bool OculusBaseDisplayPlugin::isSupported() const {
    return oculusAvailable();
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void OculusBaseDisplayPlugin::customizeContext() {
    glewExperimental = true;
    GLenum err = glewInit();
    glGetError(); // clear the potential error from glewExperimental
    Parent::customizeContext();
}

#define DEFAULT_NEAR_CLIP 0.01f
#define DEFAULT_FAR_CLIP 10000.0f

bool OculusBaseDisplayPlugin::internalActivate() {
    _session = acquireOculusSession();
    if (!_session) {
        return false;
    }

    _hmdDesc = ovr_GetHmdDesc(_session);

    glm::uvec2 eyeSizes[2];
    _viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

    _ipd = 0;
    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_ClipRangeOpenGL);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);
        _eyeOffsets[eye] = glm::translate(mat4(), toGlm(erd.HmdToEyeOffset));
        eyeSizes[eye] = toGlm(ovr_GetFovTextureSize(_session, eye, erd.Fov, 1.0f));
        _viewScaleDesc.HmdToEyeOffset[eye] = erd.HmdToEyeOffset;
        _ipd += glm::abs(glm::length(toGlm(erd.HmdToEyeOffset)));
    });

    auto combinedFov = _eyeFovs[0];
    combinedFov.LeftTan = combinedFov.RightTan = std::max(combinedFov.LeftTan, combinedFov.RightTan);
    _cullingProjection = toGlm(ovrMatrix4f_Projection(combinedFov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_ClipRangeOpenGL));

    _renderTargetSize = uvec2(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));

    memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = _sceneLayer.Viewport[eye].Size = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
        _sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });

    //    // Parent class relies on our _session intialization, so it must come after that.
    //    memset(&_overlayLayer, 0, sizeof(ovrLayerQuad));
    //    _overlayLayer.Header.Type = ovrLayerType_Quad;
    //    _overlayLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    //
    //    if (!OVR_SUCCESS(ovr_ConfigureTracking(_session,
    //        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
    //        qFatal("Could not attach to sensor device");
    //    }

    // This must come after the initialization, so that the values calculated 
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void OculusBaseDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
    releaseOculusSession();
    _session = nullptr;
}
