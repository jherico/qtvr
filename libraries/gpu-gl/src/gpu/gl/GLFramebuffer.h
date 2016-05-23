//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLFramebuffer_h
#define hifi_gpu_gl_GLFramebuffer_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLFramebuffer : public GLObject<Framebuffer> {
public:
    template <typename GLFramebufferType>
    static GLFramebufferType* sync(const Framebuffer& framebuffer) {
        GLFramebufferType* object = Backend::getGPUObject<GLFramebufferType>(framebuffer);

        bool needsUpate { false };
        if (!object ||
            framebuffer.getDepthStamp() != object->_depthStamp ||
            framebuffer.getColorStamps() != object->_colorStamps) {
            needsUpate = true;
        }

        // If GPU object already created and in sync
        if (!needsUpate) {
            return object;
        } else if (framebuffer.isEmpty()) {
            // NO framebuffer definition yet so let's avoid thinking
            return nullptr;
        }

        // need to have a gpu object?
        if (!object) {
            // All is green, assign the gpuobject to the Framebuffer
            object = new GLFramebufferType(framebuffer);
            Backend::setGPUObject(framebuffer, object);
            (void)CHECK_GL_ERROR();
        }

        object->update();
        return object;
    }

    template <typename GLFramebufferType>
    static GLuint getId(const Framebuffer& framebuffer) {
        GLFramebufferType* fbo = sync<GLFramebufferType>(framebuffer);
        if (fbo) {
            return fbo->_id;
        } else {
            return 0;
        }
    }

    const GLuint& _fbo { _id };
    std::vector<GLenum> _colorBuffers;
    Stamp _depthStamp { 0 };
    std::vector<Stamp> _colorStamps;

protected:
    GLenum _status { GL_FRAMEBUFFER_COMPLETE };
    virtual void update() = 0;
    bool checkStatus(GLenum target) const;

    GLFramebuffer(const Framebuffer& framebuffer, GLuint id) : GLObject(framebuffer, id) {}
    ~GLFramebuffer() { if (_id) { glDeleteFramebuffers(1, &_id); } };

};

} }


#endif
