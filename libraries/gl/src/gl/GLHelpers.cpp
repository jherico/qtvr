#include "GLHelpers.h"

#include <mutex>

#include <QtGui/QSurfaceFormat>

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(DEFAULT_GL_DEPTH_BUFFER_BITS);
        format.setStencilBufferSize(DEFAULT_GL_STENCIL_BUFFER_BITS);
        format.setVersion(DEFAULT_GL_MAJOR_VERSION, DEFAULT_GL_MINOR_VERSION);
#ifdef DEBUG
        format.setOption(QSurfaceFormat::DebugContext);
#endif
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}
