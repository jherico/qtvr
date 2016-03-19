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
#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>

#include <gl/OglplusHelpers.h>
#include <GLMHelpers.h>
#include <gl/ThreadedGLCanvas.h>

#include "Shadertoy.h"
#include "types/Input.h"
#include "types/Shader.h"


namespace shadertoy {
    using TexturePair = std::array<TexturePtr, 2>;

    class Renderer : public QObject {
        Q_OBJECT;
        //Q_PROPERTY(QSize size MEMBER _size NOTIFY sizeChanged);
        Q_PROPERTY(float scale MEMBER _renderScale NOTIFY scaleChanged);
        Q_PROPERTY(float fps MEMBER _fps NOTIFY fpsChanged);
        Q_PROPERTY(int pixels MEMBER _pixels NOTIFY pixelsChanged);
        Q_PROPERTY(int gpuFrameTime MEMBER _gpuFrameTime NOTIFY gpuFrameTimeChanged);

    signals:
        void compileError(const QString & source);
        void compileFailure(const QString & source);
        void compileSuccess();
        void sizeChanged();
        void scaleChanged();
        void fpsChanged();
        void pixelsChanged();
        void gpuFrameTimeChanged();

    public slots:
        void build();
        void setShader(const QVariant& shader);
        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(const QPoint& point);
        void mousePressed(const QPoint& point);
        void mouseReleased();
        void restart();
        void pause();

    public:
        void setup(const glm::uvec2& size);
        void render();
        Shader* shader();
        void setShader(Shader* newShader);
        void setShaderCode(Renderpass::Output output, const QString& code);

        void setTargetResolution(const uvec2& targetResolution);
        void setScale(float scale);

    protected:
        void setRenderScale(float newScale);
        void updateUniforms();
        void initTextureCache();
        void resize();

        Shader* _shader{ nullptr };
        QElapsedTimer _shaderTimer;

        // The fragment shader used to render the shadertoy effect, as loaded
        // from a preset or created or edited by the user
        bool _resolutionDirty { true };
        // The size of the output framebuffer
        uvec2 _targetResolution;
        bool _targetResolutionDirty { true };
        int _targetGpuTime { 0 };
        // The current rendering resolution (_size * _scale)
        uvec2 _renderResolution;
        // Equal to or half the width of the render resolution (depending on stereo settings)
        uvec2 _eyeRenderResolution; 

        // The shadertoy rendering resolution scale.  1.0 means full resolution
        // as defined by the Oculus SDK as the ideal offscreen resolution
        // pre-distortion
        float _renderScale { 0.5f };

        float _hmdScale { 1.0f };
        float _fps { 0.0f };
        int _gpuFrameTime { 0 };
        int _pixels { 0 };
        int _targetFramerate { 60 };

        // The shadertoy rendering resolution scale.  1.0 means full resolution
        // as defined by the Oculus SDK as the ideal offscreen resolution
        // pre-distortion
        int32_t _shaderFrame { 0 };
        int32_t _uboAlignment { 0 };

        QByteArray _shadertoyInputs;
        QByteArray _keyboardState;
        ThreadedGLCanvas* _backgroundContext { nullptr };
        std::atomic<bool> _texturesLoaded { false };

        vec4 _mouse { 0 };

        // UBO container
        BufferPtr _uniformsBuffer;
        // Geometry for the skybox used to render the scene
        ShapeWrapperPtr _skybox;

        // Framebuffers and textures for multipass rendering
        std::array<FramebufferPtr, 2> _bufferFramebuffers[4];

        // Framebuffer and texture for final result
        FramebufferPtr _imageFramebuffer;
        TexturePtr _imageTexture;

        // Simple 2D plane rendering
        ProgramPtr _planeProgram;
        ShapeWrapperPtr _plane;
        std::shared_ptr<oglplus::Query> _query;
    };

}

