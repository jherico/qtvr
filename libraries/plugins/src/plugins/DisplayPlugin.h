//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QSize>
#include <QtCore/QPoint>

#include <GLMHelpers.h>

#include "Plugin.h"

class QImage;

enum Eye {
    Left,
    Right
};

/*
 * Helper method to iterate over each eye
 */
template <typename F>
void for_each_eye(F f) {
    f(Left);
    f(Right);
}

/*
 * Helper method to iterate over each eye, with an additional lambda to take action between the eyes
 */
template <typename F, typename FF>
void for_each_eye(F f, FF ff) {
    f(Eye::Left);
    ff();
    f(Eye::Right);
}

class QWindow;

#define AVERAGE_HUMAN_IPD 0.064f

class DisplayPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    virtual int getHmdScreen() const { return -1; }
    /// By default, all HMDs are stereo
    virtual bool isStereo() const { return isHmd(); }
    virtual bool isThrottled() const { return false; }
    virtual float getTargetFrameRate() { return 0.0f; }

    // Rendering support

    // Stop requesting renders, but don't do full deactivation
    // needed to work around the issues caused by Oculus 
    // processing messages in the middle of submitFrame
    virtual void stop() = 0;

    /**
     *  Sends the scene texture to the display plugin.
     */
    virtual void submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) = 0;

    /**
    *  Sends the scene texture to the display plugin.
    */
    virtual void submitOverlayTexture(uint32_t overlayTexture, const glm::uvec2& overlaySize) = 0;

    // Does the rendering surface have current focus?
    virtual bool hasFocus() const = 0;

    // The size of the rendering target (may be larger than the device size due to distortion)
    virtual glm::uvec2 getRecommendedRenderSize() const = 0;

    // The size of the UI
    virtual glm::uvec2 getRecommendedUiSize() const {
        return getRecommendedRenderSize();
    }

    // By default the aspect ratio is just the render size
    virtual float getRecommendedAspectRatio() const {
        return aspect(getRecommendedRenderSize());
    }

    // Stereo specific methods
    virtual glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const { return baseProjection; }
    virtual glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const { return baseProjection; }

    // Fetch the most recently displayed image as a QImage
    virtual QImage getScreenshot() const = 0;

    // HMD specific methods
    // TODO move these into another class?
    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const {
        static const glm::mat4 transform; return transform;
    }

    virtual glm::mat4 getHeadPose(uint32_t frameIndex) const {
        static const glm::mat4 pose; return pose;
    }

    // Needed for timewarp style features
    virtual void setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
        // NOOP
    }

    virtual float getIPD() const { return AVERAGE_HUMAN_IPD; }

    virtual void abandonCalibration() {}
    virtual void resetSensors() {}
    virtual float devicePixelRatio() { return 1.0f; }
    virtual float presentRate() { return -1.0f; }
    uint32_t presentCount() const { return _presentedFrameIndex; }
    static const QString& MENU_PATH();

signals:
    void recommendedFramebufferSizeChanged(const QSize & size);

protected:
    void incrementPresentCount() { ++_presentedFrameIndex; }

private:
    std::atomic<uint32_t> _presentedFrameIndex;
};

