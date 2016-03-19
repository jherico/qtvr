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

#include "Renderer.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QGLWidget>

#include <PathUtils.h>
#include <gl/GLWindow.h>
#include <gl/OffscreenGLCanvas.h>
#include <plugins/DisplayPlugin.h>
#include <MatrixStack.h>
#include <FileUtils.h>
#include <Platform.h>
#include <shared/NsightHelpers.h>
#include "../Application.h"


enum Uniforms {
    Projection = 0,
    ModelView = 1,
};

enum Attributes {
    Position = 0,
};

enum Bindings {
    ShadertoyStatic = 1,
    ShadertoyVariable = 2
};

static const uvec2 VR_2D_RESOLUTION { 800, 450 };
static const float VR_2D_ASPECT = (float)VR_2D_RESOLUTION.x / (float)VR_2D_RESOLUTION.y;

static const char * VERTEX_SHADER = R"SHADER(#version 450 core

layout(location = 0) uniform mat4 Projection = mat4(1);
layout(location = 1) uniform mat4 ModelView = mat4(1);

layout(location = 0) in vec3 Position;

out vec3 RayDirection;

void main() {
  RayDirection = Position;
  gl_Position = Projection * ModelView * vec4(Position, 1);
}
)SHADER";

static const char* const SHADER_HEADER = R"SHADER(#version 450 core
layout (binding = 1, std140) uniform Shadertoy
{
    float     iGlobalTime;           // shader playback time (in seconds)
    float     iTimeDelta;            // render time (in seconds)
    float     iSampleRate;           // sound sample rate (i.e., 44100)
    int       iFrame;                // Shader playback frame
    float     iChannelTime[4];       // channel playback time (in seconds)
    vec3      iResolution;           // viewport resolution (in pixels)
    vec3      iChannelResolution[4]; // channel resolution (in pixels)
    vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
    vec4      iDate;                 // (year, month, day, time in seconds)
};

uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

in vec3 RayDirection;
out vec4 FragColor;
#line 1
)SHADER";

static const char * FOOTER_2D = R"SHADER(
void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    if (fragCoord.x >= iResolution.x) {
        fragCoord.x -= iResolution.x;
    } 
    mainImage(FragColor, fragCoord);
}
)SHADER";

//void mainVR( out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir );
static const char * FOOTER_VR = R"SHADER(
layout (location = 3) uniform vec3 RayOrigin = vec3(0.0, 0.0, 0.0);
layout (location = 4) uniform vec2 FragCoordOffset = vec2(0.0, 0.0);
void main() {
    vec2 fragCoord = gl_FragCoord.xy + FragCoordOffset;
    if (fragCoord.x > iResolution.x) {
        fragCoord.x -= iResolution.x;
    } 
    mainVR(FragColor, fragCoord, RayOrigin, normalize(RayDirection));
}
)SHADER";

static const char * SIMPLE_TEXTURED_VS = R"VS(#version 450 core
#pragma line __LINE__

layout(location = 0) uniform mat4 Projection = mat4(1);
layout(location = 1) uniform mat4 ModelView = mat4(1);

layout(location = 8) uniform  float uTextureScale = 1.0;

in vec3 Position;
in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord * uTextureScale;
}

)VS";

static const char * SIMPLE_TEXTURED_FS = R"FS(#version 410 core
#pragma line __LINE__

uniform sampler2D sampler;

in vec2 vTexCoord;
out vec4 FragColor;

void main() {
    FragColor = texture(sampler, vTexCoord);
}

)FS";

struct ShadertoyInputs {
    float     iGlobalTime;           // shader playback time (in seconds)
    float     iTimeDelta;            // render time (in seconds)
    // FIXME move to non-per-frame uniforms
    float     iSampleRate;           // sound sample rate (i.e., 44100)
    int       iFrame;                // Shader playback frame
    int       _padding[12];
    float     iChannelTime[4];       // channel playback time (in seconds)
    // FIXME move to non-per-frame uniforms
    vec4      iResolution;           // viewport resolution (in pixels)
    // FIXME move to non-per-frame uniforms
    vec4      iChannelResolution[4];   // channel resolution (in pixels)
    vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
    vec4      iDate;                 // (year, month, day, time in seconds)
};

static const QStringList TEXTURES({
    "/presets/tex00.jpg",
    "/presets/tex01.jpg",
    "/presets/tex02.jpg",
    "/presets/tex03.jpg",
    "/presets/tex04.jpg",
    "/presets/tex05.jpg",
    "/presets/tex06.jpg",
    "/presets/tex07.jpg",
    "/presets/tex08.jpg",
    "/presets/tex09.jpg",
    "/presets/tex10.png",
    "/presets/tex11.png",
    "/presets/tex12.png",
    "/presets/tex14.png",
    "/presets/tex15.png",
    "/presets/tex16.png",
    "/presets/tex17.jpg",
    "/presets/tex18.jpg",
    "/presets/tex19.png",
    "/presets/tex20.jpg",
});

static const QStringList CUBEMAPS({
    "/presets/cube00_%1.jpg",
    "/presets/cube01_%1.png",
    "/presets/cube02_%1.jpg",
    "/presets/cube03_%1.png",
    "/presets/cube04_%1.png",
    "/presets/cube05_%1.png",
});

static const QStringList BUFFERS({
    "/presets/previz/buffer00.png",
    "/presets/previz/buffer01.png",
    "/presets/previz/buffer02.png",
    "/presets/previz/buffer03.png"
});

static const QString KEYBOARD = "keyboard";
static const QString MIC = "mic";
static const QString WEBCAM = "webcam";

using namespace shadertoy;

struct CachedTexture {
    uvec2 resolution;
    TexturePair textures;
};

QHash<QString, CachedTexture> cachedTextures;

struct InputGL {
    Input* input { nullptr };
    vec3 resolution;
    uvec2 size;
    oglplus::TextureTarget target { oglplus::TextureTarget::_2D };
    TexturePair textures;

    oglplus::TextureMinFilter minFilter { oglplus::TextureMinFilter::Nearest };
    oglplus::TextureMagFilter magFilter { oglplus::TextureMagFilter::Nearest };
    oglplus::TextureWrap wrap { oglplus::TextureWrap::ClampToEdge };
    bool flip { true };
    bool srgb { false };
    oglplus::PixelDataType format { oglplus::PixelDataType::Byte };

    void bind(bool even) const {
        if (!input) {
            return;
        }

        switch (input->ctype) {
        case Input::AUDIO:
        case Input::SOUNDCLOUD:
        case Input::MIC:
        case Input::WEBCAM:
        case Input::VIDEO:
            return;
        }

        auto& texture = even ? textures[0] : (textures[1] ? textures[1] : textures[0]);
        if (!texture || input->channel == -1) {
            qFatal("Invalid input texture");
        }

        using namespace oglplus;
        Texture::Active(input->channel);
        texture->Bind(target);
        // FIXME excessive 
        Texture::WrapS(target, wrap);
        Texture::WrapT(target, wrap);
        Texture::WrapR(target, wrap);
        Texture::MinFilter(target, minFilter);
        Texture::MagFilter(target, magFilter);
    }

    static InputGL build(Input* input, int bufferIndex = -1) {
        InputGL result;
        result.input = input;
        CachedTexture cachedTexture;
        switch (input->ctype) {
        case Input::TEXTURE:
        case Input::BUFFER:
            if (!cachedTextures.contains(input->src)) {
                throw std::runtime_error("Could not find cached texture");
            }
            cachedTexture = cachedTextures[input->src];
            break;

        case Input::CUBEMAP:
            if (!cachedTextures.contains(input->src)) {
                throw std::runtime_error("Could not find cached texture");
            }
            cachedTexture = cachedTextures[input->src];
            result.target = oglplus::TextureTarget::CubeMap;
            break;

        case Input::KEYBOARD:
            cachedTexture = cachedTextures[KEYBOARD];
            break;

        case Input::MIC:
            cachedTexture = cachedTextures[MIC];
            break;

        case Input::WEBCAM:
            cachedTexture = cachedTextures[WEBCAM];
            break;
        }

        result.textures = cachedTexture.textures;
        result.size = cachedTexture.resolution;
        result.resolution = vec3(result.size, 1);

        using namespace oglplus;
        switch (input->wrap) {
        case Input::REPEAT:
            result.wrap = TextureWrap::Repeat;
            break;

        case Input::CLAMP:
        default:
            result.wrap = TextureWrap::ClampToEdge;
            break;
        }

        switch (input->filter) {
        case Input::MIPMAP:
            result.minFilter = TextureMinFilter::LinearMipmapLinear;
            result.magFilter = TextureMagFilter::Linear;
            break;

        case Input::LINEAR:
            result.minFilter = TextureMinFilter::Linear;
            result.magFilter = TextureMagFilter::Linear;
            break;

        case Input::NEAREST:
        default:
            result.minFilter = TextureMinFilter::Nearest;
            result.magFilter = TextureMagFilter::Nearest;
            break;
        }
        return result;
    }
};

struct RenderpassGL {
    Renderpass* renderpass { nullptr };
    ProgramPtr program;
    ProgramPtr vrProgram;
    // The currently active input channels
    InputGL inputs[4];
    std::array<FramebufferPtr, 2> outputs;
    static VertexShaderPtr _vertexShader;

    void bind(bool even) {
        outputs[even ? 0 : 1]->Bind(oglplus::FramebufferTarget::Draw);
        for (const auto& input : inputs) {
            input.bind(even);
        }
    }

    static ProgramPtr buildShader(const QString& source) {
        using namespace oglplus;
        QByteArray qb = source.toLocal8Bit();
        GLchar * fragmentSource = (GLchar*)qb.data();
        StrCRef src(fragmentSource);
        if (!_vertexShader) {
            _vertexShader = std::make_shared<VertexShader>();
            try {
                _vertexShader->Source(VERTEX_SHADER);
                _vertexShader->Compile();
            } catch (std::runtime_error& err) {
                qDebug() << err.what();
            }
            Platform::addShutdownHook([&] {
                _vertexShader.reset();
            });
        }
        FragmentShaderPtr newFragmentShader(new FragmentShader());
        newFragmentShader->Source(GLSLSource(src));
        newFragmentShader->Compile();
        ProgramPtr program = std::make_shared<Program>();
        program->AttachShader(*_vertexShader);
        program->AttachShader(*newFragmentShader);
        program->Link();
        program->Bind();
        auto activeUniforms = getActiveUniforms(program);
        for (int i = 0; i < 4; ++i) {
            const char * uniformName = Shadertoy::UNIFORM_CHANNELS[i];
            if (activeUniforms.count(uniformName)) {
                QOpenGLContext::currentContext()->functions()->glUniform1i(activeUniforms[uniformName], i);
            }
        }
        return program;
    }

    static RenderpassGL build(Renderpass* pass) {
        RenderpassGL result;
        result.renderpass = pass;
        using namespace oglplus;
        QString header = SHADER_HEADER;

        for (auto input : pass->_inputs) {
            if (input->ctype == Input::CUBEMAP) {
                auto oldString = QString().sprintf("uniform sampler2D iChannel%d;", input->channel);
                auto newString = QString().sprintf("uniform samplerCube iChannel%d;", input->channel);
                header.replace(oldString, newString);
            }
        }

        QString source = pass->code;
        source.
            replace(QRegExp("\\t"), "  ").
            replace(QRegExp("\\bgl_FragColor\\b"), "FragColor").
            replace(QRegExp("\\bfilter\\b"), "filter_").
            replace(QRegExp("\\bsmooth\\b"), "smooth_").
            replace(QRegExp("\\btexture2D\\b"), "texture").
            replace(QRegExp("\\bchar\\b"), "char_").
            replace(QRegExp("\\btextureCube\\b"), "texture");
        source.insert(0, header);
        result.program = buildShader(source + FOOTER_2D);
        
        if (source.contains(Shadertoy::VR_MARKER)) {
            result.vrProgram = buildShader(source + FOOTER_VR);
        }

        for (auto& input : pass->_inputs) {
            auto channel = input->channel;
            result.inputs[channel] = InputGL::build(input);
        }
        return result;
    }
};

VertexShaderPtr RenderpassGL::_vertexShader;

struct ShaderGL {
    using Pointer = std::shared_ptr<ShaderGL>;
    Shader* shader { nullptr };
    std::list<RenderpassGL> passes;
    bool vrShader { false };

    static ShaderGL build(Shader* shader) {
        Renderpass* imagePass = nullptr;
        ShaderGL result;
        result.shader = shader;
        for (auto pass : shader->_renderpass) {
            if (pass->output == Renderpass::IMAGE) {
                imagePass = pass;
                continue;
            }
            if (pass->output != Renderpass::BUFFER_A) {
                continue;
            }

            auto renderpass = RenderpassGL::build(pass);
            switch (result.passes.size()) {
                case 0: break;
                case 1: pass->output = Renderpass::BUFFER_B; break;
                case 2: pass->output = Renderpass::BUFFER_C; break;
                case 3: pass->output = Renderpass::BUFFER_D; break;
                default: throw std::runtime_error("Too many buffer passes");
            }
            result.passes.push_back(renderpass);
        }

        if (!imagePass) {
            throw std::runtime_error("Unable to find an image output shader");
        }
        result.vrShader = imagePass->code.contains(Shadertoy::VR_MARKER);
        result.passes.push_back(RenderpassGL::build(imagePass));
        return  result;
    }
};

using ShaderGLPtr = ShaderGL::Pointer;
ShaderGLPtr currentShadertoy;

void Renderer::setShader(const QVariant& shader) {
    _shader = qvariant_cast<::Shader*>(shader);
}

void Renderer::setup(const uvec2& size) {
    qApp->makePrimaryRenderingContextCurrent();
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uboAlignment);

    auto inputsSize = sizeof(ShadertoyInputs);
    auto inputsUboSize = inputsSize + (_uboAlignment - (inputsSize % _uboAlignment));
    // Maximum number of passes
    _shadertoyInputs.resize(static_cast<int>(inputsUboSize * 5));
    _shadertoyInputs.fill(0);

    compileProgram(_planeProgram, SIMPLE_TEXTURED_VS, SIMPLE_TEXTURED_FS);
    _plane = loadPlane(_planeProgram, VR_2D_ASPECT);

    {
        using namespace oglplus;
        _uniformsBuffer = std::make_shared<oglplus::Buffer>();
        _uniformsBuffer->Bind(BufferTarget::Uniform);
        Buffer::Data(BufferTarget::Uniform, _shadertoyInputs.size(), _shadertoyInputs.data(), BufferUsage::StreamDraw);
        Q_ASSERT(QOpenGLContext::currentContext());

        _imageTexture = std::make_shared<Texture>();
        Context::Bound(TextureTarget::_2D, *_imageTexture)
            .Image2D(0, PixelDataInternalFormat::RGBA16F, 10, 10, 0, PixelDataFormat::RGBA, PixelDataType::Float, nullptr);
        _imageFramebuffer = std::make_shared<Framebuffer>();
        _imageFramebuffer->Bind(Framebuffer::Target::Draw);
        _imageFramebuffer->AttachTexture(Framebuffer::Target::Draw, FramebufferAttachment::Color, *_imageTexture, 0);

        for (int i = 0; i < BUFFERS.size(); ++i) {
            auto& cachedTexture = cachedTextures[BUFFERS.at(i)];
            for (int j = 0; j < 2; ++j) {
                auto& texture = cachedTexture.textures[j];
                texture = std::make_shared<Texture>();
                Context::Bound(TextureTarget::_2D, *texture)
                    .Image2D(0, PixelDataInternalFormat::RGBA16F, 10, 10, 0, PixelDataFormat::RGBA, PixelDataType::Float, nullptr);
                // use odd textures on even framebuffers
                auto& framebuffer = _bufferFramebuffers[i][j == 0 ? 1 : 0];
                framebuffer = std::make_shared<Framebuffer>();
                framebuffer->Bind(Framebuffer::Target::Draw);
                framebuffer->AttachTexture(Framebuffer::Target::Draw, FramebufferAttachment::Color, *texture, 0);
            }
        }

        {
            // "/presets/previz/keyboard.png"
            CachedTexture& cachedTexture = cachedTextures[KEYBOARD];
            cachedTexture.resolution = uvec2(256, 3);
            cachedTexture.textures[0] = std::make_shared<Texture>();
            _keyboardState.resize(256 * 3);
            _keyboardState.fill(0);
            Context::Bound(TextureTarget::_2D, *cachedTexture.textures[0])
                .Image2D(0, PixelDataInternalFormat::Red, 256, 3, 0, PixelDataFormat::Red, PixelDataType::UnsignedByte, nullptr);
        }
    }

    _backgroundContext = new ThreadedGLCanvas(qApp->getPrimaryRenderingContext());
    qApp->makePrimaryRenderingContextCurrent();

    // VR shader 
    //updateShader(globalModel->_cache->fetchShader("Xs3Gzf"));
    // combustible vornoi
    //updateShader(globalModel->_cache->fetchShader("4tlSzl"));
    //updateShader(globalModel->_cache->fetchShader("Xst3Dj"));
    //updateShader(globalModel->_cache->fetchShader("XddGWj"));
    //updateShader(globalModel->_cache->fetchShader("4ddGDB"));
    //updateShader(globalModel->_cache->fetchShader("test"));
    //build();

    auto shader = new Shader(this);
    auto defaultShaderPath = PathUtils::resourcesPath() + "/misc/defaultShadertoy.json";
    auto defaultShader = ::Shader::parseFile(defaultShaderPath);
    setShader(defaultShader);
    build();

    Platform::addShutdownHook([&] {
        currentShadertoy.reset();
        _skybox.reset();
        _planeProgram.reset();
        _plane.reset();
        _uniformsBuffer.reset();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                _bufferFramebuffers[i][j].reset();
            }
        }
        _imageFramebuffer.reset();
        _imageTexture.reset();
        cachedTextures.clear();
    });
    setTargetResolution(size);
    resize();
    initTextureCache();
}

//initTextureCache();
class LambdaRunnable : public QRunnable {
public:
    LambdaRunnable(std::function<void()> f) : _f(f) {
        setAutoDelete(true);
    }

    void run() override {
        _f();
    }

private:
    std::function<void()> _f;
};

void Renderer::initTextureCache() {
    _backgroundContext->execute([=] {
        using namespace oglplus;
        for (int i = 0; i < TEXTURES.size(); ++i) {
            QString path = TEXTURES.at(i);
            qDebug() << "Loading texture from " << path;
            CachedTexture& cachedTexture = cachedTextures[path];
            cachedTexture.textures[0] = load2dTexture(PathUtils::resourcesPath() + path, cachedTexture.resolution);
        }

        for (int i = 0; i < CUBEMAPS.size(); ++i) {
            QString pathTemplate = CUBEMAPS.at(i);
            QString path = pathTemplate.arg(0);
            qDebug() << "Processing path " << path;
            CachedTexture& cachedTexture = cachedTextures[path];
            cachedTexture.textures[0] = loadCubemapTexture([&](int face) {
                QString texturePath = pathTemplate.arg(face);
                QImage image = QImage(":" + texturePath).mirrored(false, true);
                cachedTexture.resolution = toGlm(image.size());
                return image;
            });
        }
        //textureLoadCanvas->doneCurrent();
        //textureLoadCanvas->deleteLater();
        glFinish();
        _texturesLoaded = true;
    });

    //static void setOutput(Renderpass::Output output, bool even) {
    //    if (output == Renderpass::IMAGE) {
    //        _intermediate->Bind(oglplus::Framebuffer::Target::Draw); // qApp->restoreDefaultFramebuffer();
    //    } else {
    //        // Use odd framebufers as outputs on even frames
    //        auto index = ((output - Renderpass::BUFFER_A) * 2) + (even ? 1 : 0);
    //        _framebuffers[index]->Bind(oglplus::Framebuffer::Target::Draw);
    //    }
    //}
}

void Renderer::resize() {
    PROFILE_RANGE(__FUNCTION__);
    if (!_targetResolution.x || !_targetResolution.y) {
        return;
    }

    auto displayPlugin = qApp->getActiveDisplayPlugin();


    auto oldRenderResolution = _renderResolution;
    auto targetResolution = _targetResolution;
    _eyeRenderResolution = _renderResolution = uvec2(vec2(targetResolution) * _renderScale);

    if (displayPlugin->isHmd()) {
        if (_targetResolutionDirty) {
            _renderScale = 0.2f;
        }
        if (!currentShadertoy->vrShader) {
            targetResolution = uvec2(800, 450);
        }
        _eyeRenderResolution.x /= 2;
        float targetFramerate = displayPlugin->getTargetFrameRate();
        // Allocate a budget of 63% of the total frametime for rendering.
        // FIXME this should be improvable by segmenting VR frames
        _targetGpuTime = (int)(1000 * (500.0f / targetFramerate));
    } else {
        _targetGpuTime = 0;
    }
    if (currentShadertoy && currentShadertoy->vrShader && displayPlugin->isHmd()) {
    }
    
    using namespace oglplus;
    if (_targetResolutionDirty) {
        for (int i = 0; i < BUFFERS.size(); ++i) {
            auto& cachedTexture = cachedTextures[BUFFERS.at(i)];
            for (int j = 0; j < 2; ++j) {
                Context::Bound(TextureTarget::_2D, *cachedTexture.textures[j])
                    .Image2D(0, PixelDataInternalFormat::RGBA16F, _targetResolution.x, _targetResolution.y, 0, PixelDataFormat::RGBA, PixelDataType::Float, nullptr);
            }
        }
        Context::Bound(TextureTarget::_2D, *_imageTexture)
            .MinFilter(TextureMinFilter::Nearest)
            .Image2D(0, PixelDataInternalFormat::RGBA, _targetResolution.x, _targetResolution.y, 0, PixelDataFormat::RGBA, PixelDataType::Float, nullptr);
    }

    if (_renderResolution != oldRenderResolution) {
        _pixels = _renderResolution.x * _renderResolution.y;
        emit pixelsChanged();
        for (int i = 0; i < BUFFERS.size(); ++i) {
            auto& cachedTexture = cachedTextures[BUFFERS.at(i)];
            cachedTexture.resolution = _renderResolution;
        }
            
        if (currentShadertoy) {
            for (auto& pass : currentShadertoy->passes) {
                for (auto& input : pass.inputs) {
                    if (input.input && input.input->ctype == Input::BUFFER) {
                        input.resolution = vec3(_renderResolution, 1.0f);
                    }
                }
            }
        }
    }
}

void Renderer::render() {
    if (!currentShadertoy || !_targetResolution.x || !_targetResolution.y) {
        return;
    }

    if (_resolutionDirty) {
        resize();
        _resolutionDirty = false;
        _targetResolutionDirty = false;
    }

    auto displayPlugin = qApp->getActiveDisplayPlugin();

    // Use odd textures as buffer inputs
    bool even = 0 == (_shaderFrame % 2);
    updateUniforms();

    auto& mv = Stacks::modelview();
    auto& pr = Stacks::projection();
    
    using namespace oglplus;
    bool queryFrame { false };
    {
        if (!_query) {
            _query = std::make_shared<Query>();
            _query->Begin(Query::Target::TimeElapsed);
            queryFrame = true;
        }
        PROFILE_RANGE(__FUNCTION__"Render");
        using namespace oglplus;
        if (displayPlugin->isHmd() && currentShadertoy->vrShader) {
            static vec3 eyeOffsets[2];
            static vec3 transformedEyeOffsets[2];
            auto headOrientation = glm::inverse(glm::quat_cast(mv.top()));
            for_each_eye([&](Eye eye) {
                eyeOffsets[eye] = vec3(displayPlugin->getEyeToHeadTransform(eye)[3]);
                transformedEyeOffsets[eye] = headOrientation * eyeOffsets[eye];
            });
            for_each_eye([&](Eye eye) {
                uvec4 vp(eye == Eye::Left ? 0 : _eyeRenderResolution.x, 0, _eyeRenderResolution.x, _eyeRenderResolution.y);
                Context::Viewport(vp.x, vp.y, vp.z, vp.w);
                pr.top() = displayPlugin->getEyeProjection(eye, mat4());
                auto eyeTransform = displayPlugin->getEyeToHeadTransform(eye);
                mv.withPush([&] {
                    auto translation = -1.0f * vec3(mv.top()[3]);
                    translation = headOrientation  * translation;
                    auto origin = translation + transformedEyeOffsets[eye];
                    mv.top()[3] = vec4(0, 0, 0, 1);
                    for (auto& pass : currentShadertoy->passes) {
                        pass.bind(even);
                        if (pass.vrProgram) {
                            pass.vrProgram->Bind();
                            if (pass.vrProgram) {
                                ProgramUniform<vec3>(*pass.vrProgram, 3).TrySet(origin);
                            }
                        }
                        static const uint8_t TILE_COUNT = 16;
                        auto& program = pass.vrProgram;
                        auto& shape = _skybox;
                        program->Use();
                        shape->Use();
                        Mat4Uniform(*program, "ModelView").Set(Stacks::modelview().top());
                        Mat4Uniform(*program, "Projection").Set(Stacks::projection().top());
                        uvec2 tileSize = _renderResolution / uvec2(TILE_COUNT);
                        glEnable(GL_SCISSOR_TEST);
                        // FIXME subdivide the view matrix and render in parts.
                        for (uint8_t x = 0; x < TILE_COUNT; ++x) {
                            for (uint8_t y = 0; y < TILE_COUNT; ++y) {
                                auto offset = tileSize * uvec2(x, y);
                                glScissor(offset.x, offset.y, tileSize.x, tileSize.y);
                                shape->Draw();
                            }
                        }
                        glDisable(GL_SCISSOR_TEST);
                        oglplus::NoProgram().Bind();
                        oglplus::NoVertexArray().Bind();
                        //renderGeometry(_skybox, pass.vrProgram ? pass.vrProgram : pass.program);
                    }
                });
            });
        } else {
            pr.top() = mat4();
            Context::Viewport(_renderResolution.x, _renderResolution.y);
            Stacks::modelview().withIdentity([&] {
                for (auto& pass : currentShadertoy->passes) {
                    pass.bind(even);
                    renderGeometry(_skybox, pass.program);
                }
            });
        }
    }

    {
        PROFILE_RANGE(__FUNCTION__" Transfer");
        qApp->restoreDefaultFramebuffer();

        if (displayPlugin->isHmd() && !currentShadertoy->vrShader) {
            PROFILE_RANGE(__FUNCTION__" Hmd");
            Context::Clear().ColorBuffer();
            //_imageFramebuffer->Bind(FramebufferTarget::Read);

            //oglplus::Context::BlitFramebuffer(
            //    0, 0, _renderResolution.x, _renderResolution.y, 
            //    0, 0, _targetResolution.x, _targetResolution.y, 
            //    BufferSelectBit::ColorBuffer, BlitFilter::Linear);
            //oglplus::Context::BlitFramebuffer(
            //    _targetResolution.x / 2, 0, _renderResolution.x, _renderResolution.y, 
            //    _targetResolution.x / 2, 0, _targetResolution.x, _targetResolution.y, 
            //    BufferSelectBit::ColorBuffer, BlitFilter::Linear);
            
            oglplus::ProgramUniform<float>(*_planeProgram, 8).TrySet(_renderScale);
            Stacks::withIdentity([&] {
                for_each_eye([&](Eye eye) {
                    uvec4 vp(eye == Eye::Left ? 0 : _targetResolution.x / 2, 0, _targetResolution.x / 2, _targetResolution.y);
                    Context::Viewport(vp.x, vp.y, vp.z, vp.w);
                    pr.top() = displayPlugin->getEyeProjection(eye, mat4());
                    mv.top() = glm::inverse(displayPlugin->getHeadPose(qApp->getFrameCount()));
                    mv.translate(vec3(0, 0, -1));
                    QOpenGLContext::currentContext()->functions()->glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, GetName(*_imageTexture));
                    renderGeometry(_plane, _planeProgram, {});
                });
            });
        } else {
            PROFILE_RANGE(__FUNCTION__" Blit");
            _imageFramebuffer->Bind(FramebufferTarget::Read);
            oglplus::Context::BlitFramebuffer(0, 0, _renderResolution.x, _renderResolution.y, 0, 0, _targetResolution.x, _targetResolution.y, BufferSelectBit::ColorBuffer, BlitFilter::Linear);
        }
    }
    if (queryFrame) {
        _query->End(Query::Target::TimeElapsed);
    }

    for (int i = 0; i < 255; ++i) {
        _keyboardState[i + 256] = 0;
    }
    ++_shaderFrame;
    if (_query && _query->ResultAvailable()) {
        GLuint64 result;
        _query->Result(result);
        _query.reset();
        result /= NSECS_PER_USEC;
        _gpuFrameTime = (uint32_t)result;
        emit gpuFrameTimeChanged();
        if (_targetGpuTime != 0) {
            // Increase slower than we decrease
            if (_gpuFrameTime < _targetGpuTime) {
                auto now = usecTimestampNow();
                static auto lastIncreaseTime = now;
                static const uint64_t MIN_INCREASE_INTERVAL = 100 * USECS_PER_MSEC;
                if (now - lastIncreaseTime > MIN_INCREASE_INTERVAL) {
                    lastIncreaseTime = now;
                    setRenderScale(_renderScale * 1.05);
                }
            } else if (_gpuFrameTime > _targetGpuTime) {
                setRenderScale(_renderScale * 0.9);
            }
        }
    }

    auto appFps = qApp->getFps();
    if (_fps != appFps) {
        _fps = appFps;
        emit fpsChanged();
    }
}

static float nsecsToSecs(uint64_t t) {
    t /= NSECS_PER_USEC;
    return (float)t / USECS_PER_SECOND;
}

void Renderer::updateUniforms() {
    using namespace oglplus;
    static auto lastTimeNSecs = _shaderTimer.nsecsElapsed();
    auto currentTimeNSecs = _shaderTimer.nsecsElapsed();
    if (currentTimeNSecs < lastTimeNSecs) {
        lastTimeNSecs = currentTimeNSecs;
    }
    auto deltaTimeNSecs = currentTimeNSecs - currentTimeNSecs;

    for (const auto& pass : currentShadertoy->passes) {
        auto offset = pass.renderpass->output * _uboAlignment;
        ShadertoyInputs& shadertoyInput = *(ShadertoyInputs*)(_shadertoyInputs.data() + offset);
        for (int i = 0; i < 4; ++i) {
            shadertoyInput.iChannelResolution[i] = vec4(pass.inputs[i].resolution, 0.0f);
        }
        shadertoyInput.iTimeDelta = nsecsToSecs(deltaTimeNSecs);
        shadertoyInput.iGlobalTime = nsecsToSecs(currentTimeNSecs);
        shadertoyInput.iFrame = _shaderFrame;
        shadertoyInput.iMouse = _mouse;
        shadertoyInput.iResolution = vec4(_eyeRenderResolution, 1, 0);
    }
    _uniformsBuffer->Bind(BufferTarget::Uniform);
    Buffer::Data(BufferTarget::Uniform, _shadertoyInputs.size(), _shadertoyInputs.data(), BufferUsage::StreamDraw);

    auto& keyboardTexture = cachedTextures[KEYBOARD].textures[0];
    Context::Bound(TextureTarget::_2D, *keyboardTexture)
        .Image2D(0, PixelDataInternalFormat::Red, 256, 3, 0, PixelDataFormat::Red, PixelDataType::UnsignedByte, _keyboardState.data());
}

void Renderer::build() {
    using namespace oglplus;
    try {
        currentShadertoy = std::make_shared<ShaderGL>(ShaderGL::build(_shader));
        if (!_skybox) {
            const auto& passes = currentShadertoy->passes;
            _skybox = loadSkybox(passes.begin()->program);
        }

        for (auto& pass : currentShadertoy->passes) {
            if (pass.renderpass->output != Renderpass::IMAGE) {
                auto baseIndex = pass.renderpass->output - Renderpass::BUFFER_A;
                // Use odd framebuffers as output on even frames
                pass.outputs = _bufferFramebuffers[baseIndex];
            } else {
                pass.outputs[0] = pass.outputs[1] = _imageFramebuffer;
            }
            auto offset = pass.renderpass->output * _uboAlignment;
            glBindBufferRange(GL_UNIFORM_BUFFER, 1, GetName(*_uniformsBuffer), offset, sizeof(ShadertoyInputs));
        }
        resize();
        restart();
        emit compileSuccess();
    } catch (const ProgramBuildError & err) {
        qWarning() << err.Log().c_str();
        emit compileError(QString(err.Log().c_str()));
    } catch (const std::runtime_error & err) {
        qWarning() << err.what();
        emit compileFailure(QString(err.what()));
    }
}

void Renderer::setTargetResolution(const uvec2& targetResolution) {
    if (targetResolution != _targetResolution) {
        _targetResolution = targetResolution;
        _targetResolutionDirty = true;
        _resolutionDirty = true;
        emit sizeChanged();
    }
}

void Renderer::setScale(float scale) {
    if (scale != _renderScale) {
        _renderScale = scale;
        _resolutionDirty = true;
        emit scaleChanged();
    }
}

Shader* Renderer::shader() {
    return _shader;
}

void Renderer::setShader(Shader* newShader) {
    if (_shader != newShader) {
        _shader = newShader;
    }
}

void Renderer::setShaderCode(Renderpass::Output output, const QString& code) {
    for (auto& pass : _shader->_renderpass) {
        if (pass && pass->output == output) {
            pass->code = code;
        }
    }
}

void Renderer::keyPressed(int key) {
    switch (key) {
        case Qt::Key_F1:
            break;
        case Qt::Key_F2:
            qApp->getActiveDisplayPlugin()->resetSensors();
            break;
        case Qt::Key_F3:
            // eye per frame
            break;
        case Qt::Key_F4:
            // build source
            break;

        case Qt::Key_F5:
            setRenderScale(std::max(_renderScale - 0.1f, 0.2f));
            break;
        case Qt::Key_F6:
            setRenderScale(std::max(_renderScale - 0.01f, 0.2f));
            break;
        case Qt::Key_F7:
            setRenderScale(std::min(_renderScale + 0.01f, 1.0f));
            break;
        case Qt::Key_F8:
            setRenderScale(std::min(_renderScale + 0.1f, 1.0f));
            break;

        case Qt::Key_F9:
        case Qt::Key_F10:
            // toggle browser
            break;
    }
    if (key >= 0 && key < 256) {
        _keyboardState[key] = 255;
        _keyboardState[key + 256] = 255;
        _keyboardState[key + 512] = 255 - _keyboardState[key + 512];
    }
}

void Renderer::keyReleased(int key) {
    _keyboardState[key] = 0;
    _keyboardState[key + 256] = 0;
}

static vec2 relativeMousePosition(const QPoint& point) {
    vec2 mouse = toGlm(point);
    mouse /= vec2(toGlm(qApp->getWindow()->size()));
    mouse.y = 1.0 - mouse.y;
    return mouse;
}

void Renderer::mouseMoved(const QPoint& point) {
    auto mouse = relativeMousePosition(point);
    mouse *= _eyeRenderResolution;
    _mouse.x = mouse.x;
    _mouse.y = mouse.y;
}

void Renderer::mousePressed(const QPoint& point) {
    auto mouse = relativeMousePosition(point);
    mouse *= _eyeRenderResolution;
    _mouse.x = _mouse.z = mouse.x;
    _mouse.y = _mouse.w = mouse.y;
}

void Renderer::mouseReleased() {
    _mouse.z = _mouse.w = 0;
}

void Renderer::restart() {
    _shaderFrame = 0;
    _shaderTimer.restart();
}

void Renderer::pause() {
}

void Renderer::setRenderScale(float newScale) {
    newScale = glm::clamp(newScale, 0.2f, 1.0f);
    if (newScale != _renderScale) {
        _renderScale = newScale;
        _resolutionDirty = true;
        emit scaleChanged();
    }
}