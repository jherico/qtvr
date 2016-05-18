//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../../DisplayPlugin.h"

#include <condition_variable>
#include <memory>

#include <QtCore/QTimer>
#include <QtGui/QImage>

#include <GLMHelpers.h>
#include <gl/OglplusHelpers.h>
#include <gl/GLEscrow.h>
#include <shared/RateCounter.h>

#define THREADED_PRESENT 1

class OpenGLDisplayPlugin : public DisplayPlugin {
protected:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Condition = std::condition_variable;
    using TextureEscrow = GLEscrow<gpu::TexturePointer>;
public:
    static void shutdownPresentThread();
    OpenGLDisplayPlugin();

    // These must be final to ensure proper ordering of operations 
    // between the main thread and the presentation thread
    bool activate() override final;
    void deactivate() override final;

    bool eventFilter(QObject* receiver, QEvent* event) override;
    bool isDisplayVisible() const override { return true; }


    void submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) override;
    void submitOverlayTexture(const gpu::TexturePointer& overlayTexture) override;

    glm::uvec2 getRecommendedRenderSize() const override {
        return getSurfacePixels();
    }

    glm::uvec2 getRecommendedUiSize() const override {
        return getSurfaceSize();
    }

    QImage getScreenshot() const override;

    float presentRate() const override;

    float newFramePresentRate() const override;

    float droppedFrameRate() const override;

protected:
#if THREADED_PRESENT
    friend class PresentThread;
#endif
    uint32_t getSceneTextureId() const;
    uint32_t getOverlayTextureId() const;

    glm::uvec2 getSurfaceSize() const;
    glm::uvec2 getSurfacePixels() const;

    void compositeLayers();
    virtual void compositeScene();
    virtual void compositeOverlay();
    virtual void compositePointer();

    virtual bool hasFocus() const override;
    
    // FIXME make thread safe?
    virtual bool isVsyncEnabled();
    virtual void enableVsync(bool enable = true);

    // These functions must only be called on the presentation thread
    virtual void customizeContext();
    virtual void uncustomizeContext();

    // Returns true on successful activation
    virtual bool internalActivate() { return true; }
    virtual void internalDeactivate() {}
    virtual void cleanupForSceneTexture(const gpu::TexturePointer& sceneTexture);
    // Plugin specific functionality to send the composed scene to the output window or device
    virtual void internalPresent();

    void withMainThreadContext(std::function<void()> f) const;

    void useProgram(const ProgramPtr& program);
    void present();
    void updateTextures();
    void drawUnitQuad();
    void swapBuffers();
    void eyeViewport(Eye eye) const;

    virtual void updateFrameData();

    QThread* _presentThread{ nullptr };
    ProgramPtr _program;
    int32_t _mvpUniform { -1 };
    int32_t _alphaUniform { -1 };
    ShapeWrapperPtr _plane;

    mutable Mutex _mutex;
    RateCounter<> _droppedFrameRate;
    RateCounter<> _newFrameRate;
    RateCounter<> _presentRate;
    QMap<gpu::TexturePointer, uint32_t> _sceneTextureToFrameIndexMap;
    uint32_t _currentPresentFrameIndex { 0 };

    gpu::TexturePointer _currentSceneTexture { 0 };
    gpu::TexturePointer _currentOverlayTexture { 0 };

    TextureEscrow _sceneTextureEscrow;
    TextureEscrow _overlayTextureEscrow;

    glm::uvec2 _overlaySize;
    bool _vsyncSupported { false };

    struct CursorData {
        QImage image;
        vec2 hotSpot;
        uvec2 size;
        uint32_t texture { 0 };
    };

    std::map<uint16_t, CursorData> _cursorsData;
    BasicFramebufferWrapperPtr _compositeFramebuffer;
    bool _lockCurrentTexture { false };

private:
    ProgramPtr _activeProgram;
};


