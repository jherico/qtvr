//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OglplusHelpers.h"

#include <set>

#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QGLWidget>

#include <oglplus/shapes/sky_box.hpp>
#include <oglplus/shapes/sphere.hpp>

#include <Platform.h>
#include <FileUtils.h>

using namespace oglplus;
using namespace oglplus::shapes;

static const char * SIMPLE_TEXTURED_VS = R"VS(#version 410 core
#pragma line __LINE__

in vec3 Position;
in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = vec4(Position, 1);
  vTexCoord = TexCoord;
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


ProgramPtr loadDefaultShader() {
    ProgramPtr result;
    compileProgram(result, SIMPLE_TEXTURED_VS, SIMPLE_TEXTURED_FS);
    return result;
}

void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs) {
    using namespace oglplus;
    try {
        result = std::make_shared<Program>();
        // attach the shaders to the program
        result->AttachShader(
            VertexShader()
            .Source(GLSLSource(vs))
            .Compile()
            );
        result->AttachShader(
            FragmentShader()
            .Source(GLSLSource(fs))
            .Compile()
            );
        result->Link();
    } catch (ProgramBuildError& err) {
        Q_UNUSED(err);
        qWarning() << err.Log().c_str();
        Q_ASSERT_X(false, "compileProgram", "Failed to build shader program");
        qFatal("%s", (const char*)err.Message);
        result.reset();
    }
}

// Return a point's cartesian coordinates on a sphere from pitch and yaw
static glm::vec3 getPoint(float yaw, float pitch) {
    return glm::vec3(glm::cos(-pitch) * (-glm::sin(yaw)),
        glm::sin(-pitch),
        glm::cos(-pitch) * (-glm::cos(yaw)));
}


class SphereSection : public DrawingInstructionWriter, public DrawMode {
public:
    using IndexArray = std::vector<GLuint>;
    using PosArray = std::vector<float>;
    using TexArray = std::vector<float>;
    /// The type of the index container returned by Indices()
    // vertex positions
    PosArray _pos_data;
    // vertex tex coords
    TexArray _tex_data;
    IndexArray _idx_data;
    unsigned int _prim_count { 0 };

public:
    SphereSection(
        const float fov,
        const float aspectRatio,
        const int slices_,
        const int stacks_) {
        //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm
        if (fov >= PI) {
            qDebug() << "TexturedHemisphere::buildVBO(): FOV greater or equal than Pi will create issues";
        }

        int gridSize = std::max(slices_, stacks_);
        int gridSizeLog2 = 1;
        while (1 << gridSizeLog2 < gridSize) {
            ++gridSizeLog2;
        }
        gridSize = (1 << gridSizeLog2) + 1;
        // Compute number of vertices needed
        int vertices = gridSize * gridSize;
        _pos_data.resize(vertices * 3);
        _tex_data.resize(vertices * 2);

        // Compute vertices positions and texture UV coordinate
        for (int y = 0; y <= gridSize; ++y) {
            for (int x = 0; x <= gridSize; ++x) {

            }
        }
        for (int i = 0; i < gridSize; i++) {
            float stacksRatio = (float)i / (float)(gridSize - 1); // First stack is 0.0f, last stack is 1.0f
            // abs(theta) <= fov / 2.0f
            float pitch = -fov * (stacksRatio - 0.5f);
            for (int j = 0; j < gridSize; j++) {
                float slicesRatio = (float)j / (float)(gridSize - 1); // First slice is 0.0f, last slice is 1.0f
                // abs(phi) <= fov * aspectRatio / 2.0f
                float yaw = -fov * aspectRatio * (slicesRatio - 0.5f);
                int vertex = i * gridSize + j;
                int posOffset = vertex * 3;
                int texOffset = vertex * 2;
                vec3 pos = getPoint(yaw, pitch);
                _pos_data[posOffset] = pos.x;
                _pos_data[posOffset + 1] = pos.y;
                _pos_data[posOffset + 2] = pos.z;
                _tex_data[texOffset] = slicesRatio;
                _tex_data[texOffset + 1] = stacksRatio;
            }
        } // done with vertices

        int rowLen = gridSize;

        // gridsize now refers to the triangles, not the vertices, so reduce by one
        // or die by fencepost error http://en.wikipedia.org/wiki/Off-by-one_error
        --gridSize;
        int quads = gridSize * gridSize;
        for (int t = 0; t < quads; ++t) {
            int x =
                ((t & 0x0001) >> 0) |
                ((t & 0x0004) >> 1) |
                ((t & 0x0010) >> 2) |
                ((t & 0x0040) >> 3) |
                ((t & 0x0100) >> 4) |
                ((t & 0x0400) >> 5) |
                ((t & 0x1000) >> 6) |
                ((t & 0x4000) >> 7);
            int y =
                ((t & 0x0002) >> 1) |
                ((t & 0x0008) >> 2) |
                ((t & 0x0020) >> 3) |
                ((t & 0x0080) >> 4) |
                ((t & 0x0200) >> 5) |
                ((t & 0x0800) >> 6) |
                ((t & 0x2000) >> 7) |
                ((t & 0x8000) >> 8);
            int i = x * (rowLen)+y;

            _idx_data.push_back(i);
            _idx_data.push_back(i + 1);
            _idx_data.push_back(i + rowLen + 1);

            _idx_data.push_back(i + rowLen + 1);
            _idx_data.push_back(i + rowLen);
            _idx_data.push_back(i);
        }
        _prim_count = quads * 2;
    }

    /// Returns the winding direction of faces
    FaceOrientation FaceWinding(void) const {
        return FaceOrientation::CCW;
    }

    typedef GLuint(SphereSection::*VertexAttribFunc)(std::vector<GLfloat>&) const;

    /// Makes the vertex positions and returns the number of values per vertex
    template <typename T>
    GLuint Positions(std::vector<T>& dest) const {
        dest.clear();
        dest.insert(dest.begin(), _pos_data.begin(), _pos_data.end());
        return 3;
    }

    /// Makes the vertex normals and returns the number of values per vertex
    template <typename T>
    GLuint Normals(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the vertex tangents and returns the number of values per vertex
    template <typename T>
    GLuint Tangents(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the vertex bi-tangents and returns the number of values per vertex
    template <typename T>
    GLuint Bitangents(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the texture coordinates returns the number of values per vertex
    template <typename T>
    GLuint TexCoordinates(std::vector<T>& dest) const {
        dest.clear();
        dest.insert(dest.begin(), _tex_data.begin(), _tex_data.end());
        return 2;
    }

    typedef VertexAttribsInfo<
        SphereSection,
        std::tuple<
        VertexPositionsTag,
        VertexNormalsTag,
        VertexTangentsTag,
        VertexBitangentsTag,
        VertexTexCoordinatesTag
        >
    > VertexAttribs;

    Spheref MakeBoundingSphere(void) const {
        GLfloat min_x = _pos_data[3], max_x = _pos_data[3];
        GLfloat min_y = _pos_data[4], max_y = _pos_data[4];
        GLfloat min_z = _pos_data[5], max_z = _pos_data[5];
        for (std::size_t v = 0, vn = _pos_data.size() / 3; v != vn; ++v) {
            GLfloat x = _pos_data[v * 3 + 0];
            GLfloat y = _pos_data[v * 3 + 1];
            GLfloat z = _pos_data[v * 3 + 2];

            if (min_x > x) min_x = x;
            if (min_y > y) min_y = y;
            if (min_z > z) min_z = z;
            if (max_x < x) max_x = x;
            if (max_y < y) max_y = y;
            if (max_z < z) max_z = z;
        }

        Vec3f c(
            (min_x + max_x) * 0.5f,
            (min_y + max_y) * 0.5f,
            (min_z + max_z) * 0.5f
            );

        return Spheref(
            c.x(), c.y(), c.z(),
            Distance(c, Vec3f(min_x, min_y, min_z))
            );
    }

    /// Queries the bounding sphere coordinates and dimensions
    template <typename T>
    void BoundingSphere(oglplus::Sphere<T>& bounding_sphere) const {
        bounding_sphere = oglplus::Sphere<T>(MakeBoundingSphere());
    }


    /// Returns element indices that are used with the drawing instructions
    const IndexArray & Indices(Default = Default()) const {
        return _idx_data;
    }

    /// Returns the instructions for rendering of faces
    DrawingInstructions Instructions(PrimitiveType primitive) const {
        DrawingInstructions instr = this->MakeInstructions();
        DrawOperation operation;
        operation.method = DrawOperation::Method::DrawElements;
        operation.mode = primitive;
        operation.first = 0;
        operation.count = _prim_count * 3;
        operation.restart_index = DrawOperation::NoRestartIndex();
        operation.phase = 0;
        this->AddInstruction(instr, operation);
        return std::move(instr);
    }

    /// Returns the instructions for rendering of faces
    DrawingInstructions Instructions(Default = Default()) const {
        return Instructions(PrimitiveType::Triangles);
    }
};

ShapeWrapperPtr loadSphereSection(const ProgramPtr& program, float fov, float aspect, int slices, int stacks) {
    using namespace oglplus;
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" }, SphereSection(fov, aspect, slices, stacks), *program)
        );
}

void TextureRecycler::setSize(const uvec2& size) {
    if (size == _size) {
        return;
    }
    _size = size;
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    std::set<Map::key_type> toDelete;
    std::for_each(_allTextures.begin(), _allTextures.end(), [&](Map::const_reference item) {
        if (!item.second._active && item.second._size != _size) {
            toDelete.insert(item.first);
        }
    });
    std::for_each(toDelete.begin(), toDelete.end(), [&](Map::key_type key) {
        _allTextures.erase(key);
    });
}

void TextureRecycler::clear() {
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    _allTextures.clear();
}

TexturePtr TextureRecycler::getNextTexture() {
    using namespace oglplus;
    if (_readyTextures.empty()) {
        TexturePtr newTexture(new Texture());
        Context::Bound(oglplus::Texture::Target::_2D, *newTexture)
            .MinFilter(TextureMinFilter::Linear)
            .MagFilter(TextureMagFilter::Linear)
            .WrapS(TextureWrap::ClampToEdge)
            .WrapT(TextureWrap::ClampToEdge)
            .Image2D(
            0, PixelDataInternalFormat::RGBA8,
            _size.x, _size.y,
            0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, nullptr
            );
        GLuint texId = GetName(*newTexture);
        _allTextures[texId] = TexInfo { newTexture, _size };
        _readyTextures.push(newTexture);
    }

    TexturePtr result = _readyTextures.front();
    _readyTextures.pop();

    GLuint texId = GetName(*result);
    auto& item = _allTextures[texId];
    item._active = true;

    return result;
}

void TextureRecycler::recycleTexture(GLuint texture) {
    Q_ASSERT(_allTextures.count(texture));
    auto& item = _allTextures[texture];
    Q_ASSERT(item._active);
    item._active = false;
    if (item._size != _size) {
        // Buh-bye
        _allTextures.erase(texture);
        return;
    }

    _readyTextures.push(item._tex);
}

struct TextureInfo {
    uvec2 size;
    TexturePtr tex;
};

typedef std::map<QUrl, TextureInfo> TextureMap;
typedef TextureMap::iterator TextureMapItr;

void loadImage(const QString& path, ImagePtr& imagePtr, bool flip) {
    QImage image = QImage(path).mirrored(false, true);
    image = QGLWidget::convertToGLFormat(image);
    using namespace oglplus;
    size_t width = image.width();
    size_t height = image.height();
    PixelDataFormat format = PixelDataFormat::RGB;
    if (image.hasAlphaChannel()) {
        format = PixelDataFormat::RGBA;
    }
    imagePtr = std::make_shared<oglplus::images::Image>(width, height, 1, 3,
        image.constBits(), format, PixelDataInternalFormat::RGBA8);
}

TextureMap & getTextureMap() {
    static TextureMap map;
    static bool registeredShutdown = false;
    if (!registeredShutdown) {
        Platform::addShutdownHook([&] {
            map.clear();
        });
        registeredShutdown = true;
    }

    return map;
}

template <typename T, typename F>
T loadOrPopulate(std::map<QString, T> & map, const QString& resource, F loader) {
    if (!map.count(resource)) {
        map[resource] = loader();
    }
    return map[resource];
}

//TextureInfo load2dTextureInternal(const std::vector<uint8_t> & data) {
//  using namespace oglplus;
//  TextureInfo result;
//  result.tex = TexturePtr(new Texture());
//  Context::Bound(TextureTarget::_2D, *result.tex)
//    .MagFilter(TextureMagFilter::Linear)
//    .MinFilter(TextureMinFilter::Linear);
//  ImagePtr image = loadImage(data);
//  result.size.x = image->Width();
//  result.size.y = image->Height();
//  // FIXME detect alignment properly, test on both OpenCV and LibPNG
//  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//  Texture::Image2D(TextureTarget::_2D, *image);
//  return result;
//}
TexturePtr load2dTexture(const QString& path) {
    uvec2 outSize;
    return load2dTexture(path, outSize);
}

TexturePtr load2dTexture(const QString& path, uvec2 & outSize) {
    QImage image = QImage(path).mirrored(false, true);
    image = QGLWidget::convertToGLFormat(image);
    using namespace oglplus;
    outSize.x = image.width();
    outSize.y = image.height();
    PixelDataFormat format = PixelDataFormat::RGB;
    if (image.hasAlphaChannel()) {
        format = PixelDataFormat::RGBA;
    }
    TexturePtr result(new oglplus::Texture());
    Context::Bound(Texture::Target::_2D, *result)
        .Image2D(0, PixelDataInternalFormat::RGBA8, outSize.x, outSize.y, 0, format, PixelDataType::UnsignedByte, image.constBits())
        .GenerateMipmap()
        .MinFilter(TextureMinFilter::Linear)
        .MagFilter(TextureMagFilter::Linear)
        .WrapS(TextureWrap::ClampToEdge)
        .WrapT(TextureWrap::ClampToEdge);
    return result;
}

TexturePtr loadCubemapTexture(std::function<QImage(int face)> dataLoader) {
    using namespace oglplus;
    TexturePtr result = TexturePtr(new Texture());
    Context::Bound(TextureTarget::CubeMap, *result)
        .MagFilter(TextureMagFilter::Linear)
        .MinFilter(TextureMinFilter::Linear)
        .WrapS(TextureWrap::ClampToEdge)
        .WrapT(TextureWrap::ClampToEdge)
        .WrapR(TextureWrap::ClampToEdge);
    for (int i = 0; i < 6; ++i) {
        QImage image = dataLoader(i);
        image = QGLWidget::convertToGLFormat(image);
        PixelDataFormat format = PixelDataFormat::RGB;
        if (image.hasAlphaChannel()) { format = PixelDataFormat::RGBA; }
        Texture::Image2D(Texture::CubeMapFace(i), 0, PixelDataInternalFormat::RGBA8, image.width(), image.height(), 0, format, PixelDataType::UnsignedByte, image.constBits());
    }
    return result;
}


ShapeWrapperPtr loadSkybox(const ProgramPtr& program) {
    using namespace oglplus;
    ShapeWrapperPtr shape = ShapeWrapperPtr(
        new shapes::ShapeWrapper(std::initializer_list<const char*>{"Position"}, shapes::SkyBox(),
        *program));
    return shape;
}

ShapeWrapperPtr loadPlane(const ProgramPtr& program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
        b[1] /= aspect;
    } else {
        a[0] *= aspect;
    }
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" },
        shapes::Plane(a, b), *program));
}

ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program) {
    using namespace oglplus;
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper(names, shapes::Sphere(), *program));
}



ProgramPtr loadProgram(const QString& vsFile, const QString& fsFile) {
    ProgramPtr result;
    compileProgram(result,
        FileUtils::readFileToString(vsFile).toUtf8().data(),
        FileUtils::readFileToString(fsFile).toUtf8().data());
    return result;
}

UniformMap getActiveUniforms(ProgramPtr & program) {
    UniformMap activeUniforms;
    auto uniformCount = program->ActiveUniforms().Size();
    for (uint32_t i = 0; i < uniformCount; ++i) {
        std::string name = program->ActiveUniforms().At(i).Name();
        activeUniforms[name] = program->ActiveUniforms().At(i).Index();
    }
    return activeUniforms;
}

template <typename Iter>
void renderGeometryWithLambdas(ShapeWrapperPtr & shape, ProgramPtr & program, Iter begin, const Iter & end) {
    program->Use();

    Mat4Uniform(*program, "ModelView").Set(Stacks::modelview().top());
    Mat4Uniform(*program, "Projection").Set(Stacks::projection().top());

    std::for_each(begin, end, [&](const std::function<void()>&f) {
        f();
    });

    auto err = glGetError();
    shape->Use();
    err = glGetError();
    shape->Draw();
    err = glGetError();

    oglplus::NoProgram().Bind();
    oglplus::NoVertexArray().Bind();
}

//void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, std::function<void()> lambda) {
//    renderGeometry(shape, program, LambdaList({ lambda }));
//}

void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const std::list<std::function<void()>> & list) {
    renderGeometryWithLambdas(shape, program, list.begin(), list.end());
}

void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program) {
    static const std::list<std::function<void()>> EMPTY_LIST;
    renderGeometryWithLambdas(shape, program, EMPTY_LIST.begin(), EMPTY_LIST.end());
}
