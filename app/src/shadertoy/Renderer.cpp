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

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

#include <MatrixStack.h>
#include <FileUtils.h>
#include <Platform.h>

using namespace oglplus;
using namespace shadertoy;

void Renderer::setup() {
    Q_ASSERT(QOpenGLContext::currentContext());
    setCurrentShader(FileUtils::readFileToString(":/shadertoys/default.fs"));
    build();
    assert(shadertoyProgram);
    skybox = loadSkybox(shadertoyProgram);
    Platform::addShutdownHook([&] {
        shadertoyProgram.reset();
        vertexShader.reset();
        fragmentShader.reset();
        skybox.reset();
    });
}

void Renderer::updateShaderSource(const QString& source) {
    setCurrentShader(source);

}
void Renderer::updateShaderInput(int channel, const QVariant& input) {
    //if (!input.isValid()) {
    //    inputs[channel] = Input::Pointer();
    //    return;
    //}
    //inputs[channel] = getTexture(input.toMap());
}

void Renderer::updateShader(const QVariant& shader) {
}

void Renderer::render() {
    if (!shadertoyProgram) {
        return;
    }
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&] {
        mv.untranslate();
        renderGeometry(skybox, shadertoyProgram, uniformLambdas);
    });
    for (int i = 0; i < 4; ++i) {
        oglplus::DefaultTexture().Active(i);
        DefaultTexture().Bind(Texture::Target::_2D);
        DefaultTexture().Bind(Texture::Target::CubeMap);
    }
    oglplus::Texture::Active(0);
}

void Renderer::updateUniforms() {
    using namespace shadertoy;
    typedef std::map<std::string, GLuint> Map;
    Map activeUniforms = getActiveUniforms(shadertoyProgram);
    shadertoyProgram->Bind();
    // UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
        const char * uniformName = Shadertoy::UNIFORM_CHANNELS[i];
        if (activeUniforms.count(uniformName)) {
            QOpenGLContext::currentContext()->functions()->glUniform1i(activeUniforms[uniformName], i);
        }
        //if (inputs[i] && inputs[i]->texture) {
        //    if (activeUniforms.count(Shadertoy::UNIFORM_CHANNEL_RESOLUTIONS[i])) {
        //        Uniform<vec3>(*shadertoyProgram, Shadertoy::UNIFORM_CHANNEL_RESOLUTIONS[i]).Set(inputs[i]->resolution);
        //    }
        //}
    }

    uniformLambdas.clear();
    if (activeUniforms.count(Shadertoy::UNIFORM_GLOBALTIME)) {
        uniformLambdas.push_back([=] {
            Uniform<GLfloat>(*shadertoyProgram, Shadertoy::UNIFORM_GLOBALTIME).Set(secTimestampNow() - startTime);
        });
    }

    if (activeUniforms.count(Shadertoy::UNIFORM_RESOLUTION)) {
        uniformLambdas.push_back([=] {
            vec3 res = vec3(resolution, 0);
            Uniform<vec3>(*shadertoyProgram, Shadertoy::UNIFORM_RESOLUTION).Set(res);
        });
    }

    if (activeUniforms.count(Shadertoy::UNIFORM_POSITION)) {
        uniformLambdas.push_back([=] {
            Uniform<vec3>(*shadertoyProgram, Shadertoy::UNIFORM_POSITION).Set(position);
        });
    }

    //for (int i = 0; i < 4; ++i) {
    //    if (activeUniforms.count(Shadertoy::UNIFORM_CHANNELS[i]) && inputs[i]) {
    //        uniformLambdas.push_back([=] {
    //            if (inputs[i]) {
    //                inputs[i]->bind(i);
    //            }
    //        });
    //    }
    //}
}

//void mainImage( out vec4 fragColor, in vec2 fragCoord );
const char * FOOTER_2D = R"SHADER(
void main() {
    mainImage(FragColor, gl_FragCoord.xy);
}
)SHADER";

//void mainVR( out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir );
const char * FOOTER_VR = R"SHADER(
void main() {
    mainVR(FragColor, gl_FragCoord.xy, vec3(0.0, 0.0, 0.0), iDir);
}
)SHADER";

const char* const SHADER_HEADER = "#version 330\n"
"uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
"uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
"uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
"uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
"uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
"uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
"uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
"uniform vec3      iPos; // Head position\n"
"in vec3 iDir; // Direction from viewer\n"
"out vec4 FragColor;\n";

const char* const LINE_NUMBER_HEADER =
"#line 1\n";


bool Renderer::tryToBuild() {
    try {
        position = vec3();
        if (!vertexShader) {
            QString vertexShaderSource = FileUtils::readFileToString(":/shadertoys/default.vs");
            vertexShader = VertexShaderPtr(new VertexShader());
            vertexShader->Source(vertexShaderSource.toLocal8Bit().constData());
            vertexShader->Compile();
        }

        QString header = SHADER_HEADER;
        //for (int i = 0; i < 4; ++i) {
        //    auto input = inputs[i];
        //    if (!input) {
        //        continue;
        //    }
        //    QString line; line.sprintf("uniform sampler%s iChannel%d;\n",
        //        input->target == Texture::Target::CubeMap ? "Cube" : "2D", i);
        //    header += line;
        //}
        
        header += LINE_NUMBER_HEADER;
        FragmentShaderPtr newFragmentShader(new FragmentShader());
        QString source = _currentShader;
        source.
            replace(QRegExp("\\t"), "  ").
            replace(QRegExp("\\bgl_FragColor\\b"), "FragColor").
            replace(QRegExp("\\btexture2D\\b"), "texture").
            replace(QRegExp("\\btextureCube\\b"), "texture");
        source.insert(0, header);
        source += FOOTER_2D;
        QByteArray qb = source.toLocal8Bit();
        GLchar * fragmentSource = (GLchar*)qb.data();
        StrCRef src(fragmentSource);
        newFragmentShader->Source(GLSLSource(src));
        newFragmentShader->Compile();
        ProgramPtr result(new Program());
        result->AttachShader(*vertexShader);
        result->AttachShader(*newFragmentShader);

        result->Link();
        shadertoyProgram.swap(result);
        if (!skybox) {
            skybox = loadSkybox(shadertoyProgram);
        }
        fragmentShader.swap(newFragmentShader);
        updateUniforms();
        startTime = secTimestampNow();
        _validShader = _currentShader;
        emit validShaderChanged();
        emit compileSuccess();
    } catch (ProgramBuildError & err) {
        qWarning() << err.Log().c_str();
        emit compileError(QString(err.Log().c_str()));
        return false;
    }
    return true;
}

void Renderer::setCurrentShader(const QString& shader) {
    if (shader != _currentShader) {
        _currentShader = shader;
        emit currentShaderChanged();
    }
}

void Renderer::build() {
    tryToBuild();
}
