//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

// FIXME support oglplus on all platforms
// For now it's a convenient helper for Windows

#include <queue>
#include <map>

#include <QtGlobal>

#include <GLMHelpers.h>
#include <MatrixStack.h>

#define OGLPLUS_USE_GLCOREARB_H 0
#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_BOOST_CONFIG 1
#define OGLPLUS_NO_SITE_CONFIG 1
#define OGLPLUS_LOW_PROFILE 1

// NOTE: oglplus does some naked "#pragma GCC" without proper platform wrapping, so we need to disable this warning.
#ifdef _WIN32
#pragma warning(push)
#pragma warning( disable : 4068 )
#endif
#include <oglplus/gl.hpp>

#include <oglplus/all.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/bound/framebuffer.hpp>
#include <oglplus/bound/renderbuffer.hpp>
#include <oglplus/shapes/wrapper.hpp>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/interop/glm.hpp>

#ifdef _WIN32
#pragma warning(pop)
#endif

#include "NumericalConstants.h"

using FramebufferPtr = std::shared_ptr<oglplus::Framebuffer>;
using RenderbufferPtr = std::shared_ptr<oglplus::Renderbuffer>;
using ShapeWrapperPtr = std::shared_ptr<oglplus::shapes::ShapeWrapper>;
using BufferPtr = std::shared_ptr<oglplus::Buffer>;
using VertexArrayPtr = std::shared_ptr<oglplus::VertexArray>;
using ProgramPtr = std::shared_ptr<oglplus::Program>;
using Mat4Uniform = oglplus::Uniform<mat4>;
using VertexShaderPtr = std::shared_ptr<oglplus::VertexShader>;
using FragmentShaderPtr = std::shared_ptr<oglplus::FragmentShader>;
using TexturePtr = std::shared_ptr<oglplus::Texture>;
using ImagePtr = std::shared_ptr<oglplus::images::Image>;
using UniformMap = std::map<std::string, uint32_t>;

ProgramPtr loadDefaultShader();
void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs);
ProgramPtr loadProgram(const QString & vsFile, const QString & fsFile);
UniformMap getActiveUniforms(ProgramPtr& program);

ShapeWrapperPtr loadPlane(const ProgramPtr& program, float aspect = 1.0f);
ShapeWrapperPtr loadSphereSection(const ProgramPtr& program, float fov = PI / 3.0f * 2.0f, float aspect = 16.0f / 9.0f, int slices = 32, int stacks = 32);
    

// A basic wrapper for constructing a framebuffer with a renderbuffer
// for the depth attachment and an undefined type for the color attachement
// This allows us to reuse the basic framebuffer code for both the Mirror
// FBO as well as the Oculus swap textures we will use to render the scene
// Though we don't really need depth at all for the mirror FBO, or even an
// FBO, but using one means I can just use a glBlitFramebuffer to get it onto
// the screen.
template <
    typename C,
    typename D
>
struct FramebufferWrapper {
    uvec2       size;
    oglplus::Framebuffer fbo;
    C           color;
    D           depth;

    FramebufferWrapper() {}

    virtual ~FramebufferWrapper() {
    }

    virtual void Init(const uvec2 & size) {
        this->size = size;
        initColor();
        initDepth();
        initDone();
    }

    template <typename F>
    void Bound(F f) {
        Bound(oglplus::Framebuffer::Target::Draw, f);
    }

    template <typename F>
    void Bound(oglplus::Framebuffer::Target target , F f) {
        fbo.Bind(target);
        onBind(target);
        f();
        onUnbind(target);
        oglplus::DefaultFramebuffer().Bind(target);
    }

    void Viewport() {
        oglplus::Context::Viewport(size.x, size.y);
    }

protected:
    virtual void onBind(oglplus::Framebuffer::Target target) {}
    virtual void onUnbind(oglplus::Framebuffer::Target target) {}

    static GLenum toEnum(oglplus::Framebuffer::Target target) {
        switch (target) {
        case oglplus::Framebuffer::Target::Draw:
            return GL_DRAW_FRAMEBUFFER;
        case oglplus::Framebuffer::Target::Read:
            return GL_READ_FRAMEBUFFER;
        default:
            Q_ASSERT(false);
            return GL_FRAMEBUFFER;
        }
    }

    virtual void initDepth() {}

    virtual void initColor() {}

    virtual void initDone() = 0;
};

struct BasicFramebufferWrapper : public FramebufferWrapper <oglplus::Texture, oglplus::Renderbuffer> {
protected:
    virtual void initDepth() override {
        using namespace oglplus;
        Context::Bound(Renderbuffer::Target::Renderbuffer, depth)
            .Storage(
            PixelDataInternalFormat::DepthComponent,
            size.x, size.y);
    }

    virtual void initColor() override {
        using namespace oglplus;
        Context::Bound(oglplus::Texture::Target::_2D, color)
            .MinFilter(TextureMinFilter::Linear)
            .MagFilter(TextureMagFilter::Linear)
            .WrapS(TextureWrap::ClampToEdge)
            .WrapT(TextureWrap::ClampToEdge)
            .Image2D(
                0, PixelDataInternalFormat::RGBA8,
                size.x, size.y,
                0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, nullptr
            );
    }

    virtual void initDone() override {
        using namespace oglplus;
        static const Framebuffer::Target target = Framebuffer::Target::Draw;
        Bound(target, [&] {
            fbo.AttachTexture(target, FramebufferAttachment::Color, color, 0);
            fbo.AttachRenderbuffer(target, FramebufferAttachment::Depth, depth);
            fbo.Complete(target);
        });
    }
};

using BasicFramebufferWrapperPtr = std::shared_ptr<BasicFramebufferWrapper>;

class TextureRecycler {
public:
    void setSize(const uvec2& size);
    void clear();
    TexturePtr getNextTexture();
    void recycleTexture(GLuint texture);

private:

    struct TexInfo {
        TexturePtr _tex;
        uvec2 _size;
        bool _active{ false };

        TexInfo() {}
        TexInfo(TexturePtr tex, const uvec2& size) : _tex(tex), _size(size) {}
    };

    using Map = std::map<GLuint, TexInfo>;
    using Queue = std::queue<TexturePtr>;

    Map _allTextures;
    Queue _readyTextures;
    uvec2 _size{ 1920, 1080 };
};

using Lambda = std::function<void()>;
using LambdaList = std::list<Lambda>;

inline void viewport(const uvec2 & size, const ivec2& position = ivec2(0)) {
  oglplus::Context::Viewport(position.x, position.y, size.x, size.y);
}

ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program);
ShapeWrapperPtr loadSkybox(const ProgramPtr& program);
ShapeWrapperPtr loadPlane(const ProgramPtr& program, float aspect);
void loadImage(const QString& path, ImagePtr& image, bool flip = true);
TexturePtr load2dTexture(const QString& path, uvec2 & outSize);
TexturePtr load2dTexture(const QString& path);
TexturePtr loadCubemapTexture(std::function<QImage(int i)> dataLoader);
void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const std::list<std::function<void()>> & list = {});

class Stacks {
public:
    static MatrixStack & projection() {
        static MatrixStack projection;
        return projection;
    }

    static MatrixStack & modelview() {
        static MatrixStack modelview;
        return modelview;
    }

    template <typename Function>
    static void withPush(MatrixStack & stack, Function f) {
        stack.withPush(f);
    }

    template <typename Function>
    static void withPush(MatrixStack & stack1, MatrixStack & stack2, Function f) {
        stack1.withPush([&] {
            stack2.withPush(f);
        });
    }

    template <typename Function>
    static void withPush(Function f) {
        withPush(projection(), modelview(), f);
    }

    template <typename Function>
    static void withIdentity(Function f) {
        withPush(projection(), modelview(), [=] {
            projection().identity();
            modelview().identity();
            f();
        });
    }
};
