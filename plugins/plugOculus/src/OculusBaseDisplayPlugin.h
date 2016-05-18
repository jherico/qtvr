//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <plugins/impl/display/hmd/HmdDisplayPlugin.h>

#include <QtCore/QTimer>

#include <OVR_CAPI_GL.h>

class OculusBaseDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;
public:
    virtual bool isSupported() const override;

    // Stereo specific methods
    virtual void resetSensors() override final;
    virtual void beginFrameRender(uint32_t frameIndex) override;
    float getTargetFrameRate() const override { return _hmdDesc.DisplayRefreshRate; }
    

protected:
    void customizeContext() override;
    bool internalActivate() override;
    void internalDeactivate() override;

protected:
    ovrSession _session { nullptr };
    ovrGraphicsLuid _luid;
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrFovPort _eyeFovs[2];
    ovrHmdDesc _hmdDesc;
    ovrLayerEyeFov _sceneLayer;
    ovrViewScaleDesc _viewScaleDesc;
    ovrLayerQuad _overlayLayer;
};
