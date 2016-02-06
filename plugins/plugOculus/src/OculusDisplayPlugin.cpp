//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

#include <QtGui/QWindow>

// FIXME get rid of this
#include <gl/Config.h>
#include <plugins/PluginApplication.h>

#include "OculusHelpers.h"


#if (OVR_MAJOR_VERSION >= 6)

// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovr_CreateSwapTextureSetGL,
// ovr_CreateMirrorTextureGL, etc
template <typename C>
struct RiftFramebufferWrapper : public FramebufferWrapper<C, char> {
    ovrSession session;
    RiftFramebufferWrapper(const ovrSession& session) : session(session) {
        color = 0;
        depth = 0;
    };

    ~RiftFramebufferWrapper() {
        destroyColor();
    }

    void Resize(const uvec2 & size) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        this->size = size;
        initColor();
        initDone();
    }

protected:
    virtual void destroyColor() {
    }

    virtual void initDepth() override final {
    }
};

// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures
struct SwapFramebufferWrapper : public RiftFramebufferWrapper<ovrSwapTextureSet*> {
    SwapFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }


    void Increment() {
        ++color->CurrentIndex;
        color->CurrentIndex %= color->TextureCount;
    }

protected:
    virtual void destroyColor() override {
        if (color) {
            ovr_DestroySwapTextureSet(session, color);
            color = nullptr;
        }
    }

    virtual void initColor() override {
        destroyColor();

        if (!OVR_SUCCESS(ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, size.x, size.y, &color))) {
            qFatal("Unable to create swap textures");
        }

        for (int i = 0; i < color->TextureCount; ++i) {
            ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
            glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    virtual void initDone() override {
    }

    virtual void onBind(oglplus::Framebuffer::Target target) override {
        ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
    }

    virtual void onUnbind(oglplus::Framebuffer::Target target) override {
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }
};


// We use a FBO to wrap the mirror texture because it makes it easier to
// render to the screen via glBlitFramebuffer
struct MirrorFramebufferWrapper : public RiftFramebufferWrapper<ovrGLTexture*> {
    MirrorFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) { }

private:
    virtual void destroyColor() override {
        if (color) {
            ovr_DestroyMirrorTexture(session, (ovrTexture*)color);
            color = nullptr;
        }
    }

    void initColor() override {
        destroyColor();
        ovrResult result = ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, size.x, size.y, (ovrTexture**)&color);
        Q_ASSERT(OVR_SUCCESS(result));
    }

    void initDone() override {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color->OGL.TexId, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};


#endif

const QString OculusDisplayPlugin::NAME("Oculus Rift");

const QString & OculusDisplayPlugin::getName() const {
    return NAME;
}

static const QString MONO_PREVIEW = "Mono Preview";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";

void OculusDisplayPlugin::activate() {
    OculusBaseDisplayPlugin::activate();
}

void OculusDisplayPlugin::customizeContext() {
    OculusBaseDisplayPlugin::customizeContext();
    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_session));
    _sceneFbo->Init(getRecommendedRenderSize());

    _mirrorFbo = MirrorFboPtr(new MirrorFramebufferWrapper(_session));
    _mirrorFbo->Init(getRecommendedRenderSize());

    // We're rendering both eyes to the same texture, so only one of the 
    // pointers is populated
    _sceneLayer.ColorTexture[0] = _sceneFbo->color;
    // not needed since the structure was zeroed on init, but explicit
    _sceneLayer.ColorTexture[1] = nullptr;


    _overlayFbo = SwapFboPtr(new SwapFramebufferWrapper(_session));
    _overlayFbo->Init(getRecommendedUiSize());
    _overlayLayer.ColorTexture = _overlayFbo->color;

    _layers[0] = &_sceneLayer.Header;
    _layers[1] = &_overlayLayer.Header;

    enableVsync(false);
    // Only enable mirroring if we know vsync is disabled
    _enablePreview = !isVsyncEnabled();
}

void OculusDisplayPlugin::uncustomizeContext() {
#if (OVR_MAJOR_VERSION >= 6)
    _sceneFbo.reset();
#endif
    OculusBaseDisplayPlugin::uncustomizeContext();
}

void OculusDisplayPlugin::internalPresent() {
    if (!_currentSceneTexture) {
        return;
    }

    using namespace oglplus;
    // Need to make sure only the display plugin is responsible for 
    // controlling vsync
    wglSwapIntervalEXT(0);

    _sceneFbo->Bound([&] {
        auto size = _sceneFbo->size;
        Context::Viewport(size.x, size.y);
        glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
        //glEnable(GL_FRAMEBUFFER_SRGB);
        GLenum err = glGetError();
        drawUnitQuad();
        //glDisable(GL_FRAMEBUFFER_SRGB);
    });
    _overlayFbo->Bound([&] {
        auto size = _overlayFbo->size;
        Context::Viewport(size.x, size.y);
        Context::ClearColor(0, 0, 0, 0);
        Context::Clear().ColorBuffer().DepthBuffer();
        Context::Enable(Capability::Blend);
        Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
        glBindTexture(GL_TEXTURE_2D, _currentOverlayTexture);
        GLenum err = glGetError();
        drawUnitQuad();
        Context::Disable(Capability::Blend);
    });


    uint32_t frameIndex { 0 };
    EyePoses eyePoses;
    {
        Lock lock(_mutex);
        Q_ASSERT(_sceneTextureToFrameIndexMap.contains(_currentSceneTexture));
        frameIndex = _sceneTextureToFrameIndexMap[_currentSceneTexture];
        Q_ASSERT(_frameEyePoses.contains(frameIndex));
        eyePoses = _frameEyePoses[frameIndex];
    }

    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Left] = eyePoses.first;
    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Right] = eyePoses.second;

    {
        ovrViewScaleDesc viewScaleDesc;
        viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
        viewScaleDesc.HmdToEyeViewOffset[0] = _eyeOffsets[0];
        viewScaleDesc.HmdToEyeViewOffset[1] = _eyeOffsets[1];

        // ...and now the new quad
        _overlayLayer.Viewport.Pos.x = 0;
        _overlayLayer.Viewport.Pos.y = 0;
        _overlayLayer.Viewport.Size.w = _overlayFbo->size.x;
        _overlayLayer.Viewport.Size.h = _overlayFbo->size.y;
        memset(&_overlayLayer.QuadPoseCenter, 0, sizeof(_overlayLayer.QuadPoseCenter));
        _overlayLayer.QuadPoseCenter.Orientation.w = 1;
        _overlayLayer.QuadPoseCenter.Position.z = -0.5f;
        _overlayLayer.QuadSize.x = 0.8f;
        _overlayLayer.QuadSize.y = 0.45f;

        ovrResult result = ovr_SubmitFrame(_session, frameIndex, &viewScaleDesc, _layers, 2);
        if (!OVR_SUCCESS(result)) {
            qDebug() << result;
        }
    }

    // screen preview mirroring
    if (_enablePreview) {
        auto windowSize = toGlm(_window->size());
        Context::Viewport(windowSize.x, windowSize.y);
        glBindTexture(GL_TEXTURE_2D, _mirrorFbo->color->OGL.TexId);
        GLenum err = glGetError();
        Q_ASSERT(0 == err);
        drawUnitQuad();
    }


    _sceneFbo->Increment();
    _overlayFbo->Increment();

    /*
    The swapbuffer call here is only required if we want to mirror the content to the screen.
    However, it should only be done if we can reliably disable v-sync on the mirror surface,
    otherwise the swapbuffer delay will interefere with the framerate of the headset
    */
    if (_enablePreview) {
        swapBuffers();
    }
}

void OculusDisplayPlugin::setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
    auto ovrPose = ovrPoseFromGlm(pose);
    {
        Lock lock(_mutex);
        if (eye == Eye::Left) {
            _frameEyePoses[frameIndex].first = ovrPose;
        } else {
            _frameEyePoses[frameIndex].second = ovrPose;
        }
    }
}
