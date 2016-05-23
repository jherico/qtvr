//
//  GeometryCache.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shapes.h"

#include <cmath>

#include <NumericalConstants.h>
#include <gpu/Format.h>
#include <gpu/Batch.h>

namespace geometry {

const int VERTICES_PER_TRIANGLE = 3;

const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::NUINT8, gpu::RGBA };

gpu::Stream::FormatPointer SOLID_STREAM_FORMAT;
gpu::Stream::FormatPointer INSTANCED_SOLID_STREAM_FORMAT;

const uint SHAPE_VERTEX_STRIDE = sizeof(glm::vec3) * 2; // vertices and normals
const uint SHAPE_NORMALS_OFFSET = sizeof(glm::vec3);
const gpu::Type SHAPE_INDEX_TYPE = gpu::UINT32;
const uint SHAPE_INDEX_SIZE = sizeof(gpu::uint32);


#if 0
static void setupVertices(gpu::BufferPointer& vertexBuffer, const VertexVector& vertices) {
    vertexBuffer->append(vertices);

    _positionView = gpu::BufferView(vertexBuffer, 0,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, POSITION_ELEMENT);
    _normalView = gpu::BufferView(vertexBuffer, SHAPE_NORMALS_OFFSET,
        vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, NORMAL_ELEMENT);
}

void GeometryCache::ShapeData::setupIndices(gpu::BufferPointer& indexBuffer, const IndexVector& indices, const IndexVector& wireIndices) {
    _indices = indexBuffer;
    if (!indices.empty()) {
        _indexOffset = indexBuffer->getSize() / SHAPE_INDEX_SIZE;
        _indexCount = indices.size();
        indexBuffer->append(indices);
    }

    if (!wireIndices.empty()) {
        _wireIndexOffset = indexBuffer->getSize() / SHAPE_INDEX_SIZE;
        _wireIndexCount = wireIndices.size();
        indexBuffer->append(wireIndices);
    }
}

void GeometryCache::ShapeData::setupBatch(gpu::Batch& batch) const {
    batch.setInputBuffer(gpu::Stream::POSITION, _positionView);
    batch.setInputBuffer(gpu::Stream::NORMAL, _normalView);
    batch.setIndexBuffer(SHAPE_INDEX_TYPE, _indices, 0);
}

void GeometryCache::ShapeData::draw(gpu::Batch& batch) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.drawIndexed(gpu::TRIANGLES, (gpu::uint32)_indexCount, (gpu::uint32)_indexOffset);
    }
}

void GeometryCache::ShapeData::drawWire(gpu::Batch& batch) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.drawIndexed(gpu::LINES, (gpu::uint32)_wireIndexCount, (gpu::uint32)_wireIndexOffset);
    }
}

void GeometryCache::ShapeData::drawInstances(gpu::Batch& batch, size_t count) const {
    if (_indexCount) {
        setupBatch(batch);
        batch.drawIndexedInstanced((gpu::uint32)count, gpu::TRIANGLES, (gpu::uint32)_indexCount, (gpu::uint32)_indexOffset);
    }
}

void GeometryCache::ShapeData::drawWireInstances(gpu::Batch& batch, size_t count) const {
    if (_wireIndexCount) {
        setupBatch(batch);
        batch.drawIndexedInstanced((gpu::uint32)count, gpu::LINES, (gpu::uint32)_wireIndexCount, (gpu::uint32)_wireIndexOffset);
    }
}

static const size_t ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT = 3;

size_t GeometryCache::getShapeTriangleCount(Shape shape) {
    return _shapes[shape]._indexCount / VERTICES_PER_TRIANGLE;
}

size_t GeometryCache::getSphereTriangleCount() {
    return getShapeTriangleCount(Sphere);
}

size_t GeometryCache::getCubeTriangleCount() {
    return getShapeTriangleCount(Cube);
}
#endif

IndexPair indexPair(Index a, Index b) {
    if (a > b) {
        std::swap(a, b);
    }
    return (((IndexPair)a) << 32) | ((IndexPair)b);
}

Solid<3> tesselate(Solid<3> solid, int count) {
    float length = glm::length(solid.vertices[0]);
    for (int i = 0; i < count; ++i) {
        Solid<3> result { solid.vertices, {} };
        result.vertices.reserve(solid.vertices.size() + solid.faces.size() * 3);
        for (size_t f = 0; f < solid.faces.size(); ++f) {
            Index baseVertex = (Index)result.vertices.size();
            const Face<3>& oldFace = solid.faces[f];
            const vec3& a = solid.vertices[oldFace[0]];
            const vec3& b = solid.vertices[oldFace[1]];
            const vec3& c = solid.vertices[oldFace[2]];
            vec3 ab = glm::normalize(a + b) * length;
            vec3 bc = glm::normalize(b + c) * length;
            vec3 ca = glm::normalize(c + a) * length;
            result.vertices.push_back(ab);
            result.vertices.push_back(bc);
            result.vertices.push_back(ca);
            result.faces.push_back(Face<3>{ { oldFace[0], baseVertex, baseVertex + 2 } });
            result.faces.push_back(Face<3>{ { baseVertex, oldFace[1], baseVertex + 1 } });
            result.faces.push_back(Face<3>{ { baseVertex + 1, oldFace[2], baseVertex + 2 } });
            result.faces.push_back(Face<3>{ { baseVertex, baseVertex + 1, baseVertex + 2 } });
        }
        solid = result;
    }
    return solid;
}

template <size_t N>
void setupFlatShape(ShapeData& shapeData, const Solid<N>& shape, gpu::BufferPointer& vertexBuffer, gpu::BufferPointer& indexBuffer) {
    Index baseVertex = (Index)(vertexBuffer->getSize() / SHAPE_VERTEX_STRIDE);
    VertexVector vertices;
    IndexVector solidIndices, wireIndices;
    IndexPairs wireSeenIndices;

    size_t faceCount = shape.faces.size();
    size_t faceIndexCount = triangulatedFaceIndexCount<N>();

    vertices.reserve(N * faceCount * 2);
    solidIndices.reserve(faceIndexCount * faceCount);

    for (size_t f = 0; f < faceCount; ++f) {
        const Face<N>& face = shape.faces[f];
        // Compute the face normal
        vec3 faceNormal = shape.getFaceNormal(f);

        // Create the vertices for the face
        for (Index i = 0; i < N; ++i) {
            Index originalIndex = face[i];
            vertices.push_back(shape.vertices[originalIndex]);
            vertices.push_back(faceNormal);
        }

        // Create the wire indices for unseen edges
        for (Index i = 0; i < N; ++i) {
            Index a = i;
            Index b = (i + 1) % N;
            auto token = indexToken(face[a], face[b]);
            if (0 == wireSeenIndices.count(token)) {
                wireSeenIndices.insert(token);
                wireIndices.push_back(a + baseVertex);
                wireIndices.push_back(b + baseVertex);
            }
        }

        // Create the solid face indices
        for (Index i = 0; i < N - 2; ++i) {
            solidIndices.push_back(0 + baseVertex);
            solidIndices.push_back(i + 1 + baseVertex);
            solidIndices.push_back(i + 2 + baseVertex);
        }
        baseVertex += (Index)N;
    }

    shapeData.setupVertices(vertexBuffer, vertices);
    shapeData.setupIndices(indexBuffer, solidIndices, wireIndices);
}

template <size_t N>
void setupSmoothShape(ShapeData& shapeData, const Solid<N>& shape, gpu::BufferPointer& vertexBuffer, gpu::BufferPointer& indexBuffer) {
    Index baseVertex = (Index)(vertexBuffer->getSize() / SHAPE_VERTEX_STRIDE);

    VertexVector vertices;
    vertices.reserve(shape.vertices.size() * 2);
    for (const auto& vertex : shape.vertices) {
        vertices.push_back(vertex);
        vertices.push_back(vertex);
    }

    IndexVector solidIndices, wireIndices;
    IndexPairs wireSeenIndices;

    size_t faceCount = shape.faces.size();
    size_t faceIndexCount = triangulatedFaceIndexCount<N>();

    solidIndices.reserve(faceIndexCount * faceCount);

    for (size_t f = 0; f < faceCount; ++f) {
        const Face<N>& face = shape.faces[f];
        // Create the wire indices for unseen edges
        for (Index i = 0; i < N; ++i) {
            Index a = face[i];
            Index b = face[(i + 1) % N];
            auto token = indexToken(a, b);
            if (0 == wireSeenIndices.count(token)) {
                wireSeenIndices.insert(token);
                wireIndices.push_back(a + baseVertex);
                wireIndices.push_back(b + baseVertex);
            }
        }

        // Create the solid face indices
        for (Index i = 0; i < N - 2; ++i) {
            solidIndices.push_back(face[i] + baseVertex);
            solidIndices.push_back(face[i + 1] + baseVertex);
            solidIndices.push_back(face[i + 2] + baseVertex);
        }
    }

    shapeData.setupVertices(vertexBuffer, vertices);
    shapeData.setupIndices(indexBuffer, solidIndices, wireIndices);
}

// The golden ratio
static const float PHI = 1.61803398874f;

const Solid<3>& tetrahedron() {
    static const auto A = vec3(1, 1, 1);
    static const auto B = vec3(1, -1, -1);
    static const auto C = vec3(-1, 1, -1);
    static const auto D = vec3(-1, -1, 1);
    static const Solid<3> TETRAHEDRON = Solid<3>{
        { A, B, C, D },
        FaceVector<3>{
            Face<3> { { 0, 1, 2 } },
                Face<3> { { 3, 1, 0 } },
                Face<3> { { 2, 3, 0 } },
                Face<3> { { 2, 1, 3 } },
        }
    }.fitDimension(0.5f);
    return TETRAHEDRON;
}

const Solid<4>& cube() {
    static const auto A = vec3(1, 1, 1);
    static const auto B = vec3(-1, 1, 1);
    static const auto C = vec3(-1, 1, -1);
    static const auto D = vec3(1, 1, -1);
    static const Solid<4> CUBE = Solid<4>{
        { A, B, C, D, -A, -B, -C, -D },
        FaceVector<4>{
            Face<4> { { 3, 2, 1, 0 } },
                Face<4> { { 0, 1, 7, 6 } },
                Face<4> { { 1, 2, 4, 7 } },
                Face<4> { { 2, 3, 5, 4 } },
                Face<4> { { 3, 0, 6, 5 } },
                Face<4> { { 4, 5, 6, 7 } },
        }
    }.fitDimension(0.5f);
    return CUBE;
}

const Solid<3>& octahedron() {
    static const auto A = vec3(0, 1, 0);
    static const auto B = vec3(0, -1, 0);
    static const auto C = vec3(0, 0, 1);
    static const auto D = vec3(0, 0, -1);
    static const auto E = vec3(1, 0, 0);
    static const auto F = vec3(-1, 0, 0);
    static const Solid<3> OCTAHEDRON = Solid<3>{
        { A, B, C, D, E, F},
        FaceVector<3> {
            Face<3> { { 0, 2, 4, } },
            Face<3> { { 0, 4, 3, } },
            Face<3> { { 0, 3, 5, } },
            Face<3> { { 0, 5, 2, } },
            Face<3> { { 1, 4, 2, } },
            Face<3> { { 1, 3, 4, } },
            Face<3> { { 1, 5, 3, } },
            Face<3> { { 1, 2, 5, } },
        }
    }.fitDimension(0.5f);
    return OCTAHEDRON;
}

const Solid<5>& dodecahedron() {
    static const float P = PHI;
    static const float IP = 1.0f / PHI;
    static const vec3 A = vec3(IP, P, 0);
    static const vec3 B = vec3(-IP, P, 0);
    static const vec3 C = vec3(-1, 1, 1);
    static const vec3 D = vec3(0, IP, P);
    static const vec3 E = vec3(1, 1, 1);
    static const vec3 F = vec3(1, 1, -1);
    static const vec3 G = vec3(-1, 1, -1);
    static const vec3 H = vec3(-P, 0, IP);
    static const vec3 I = vec3(0, -IP, P);
    static const vec3 J = vec3(P, 0, IP);

    static const Solid<5> DODECAHEDRON = Solid<5>{
        { 
            A,  B,  C,  D,  E,  F,  G,  H,  I,  J, 
            -A, -B, -C, -D, -E, -F, -G, -H, -I, -J, 
        },
        FaceVector<5> {
            Face<5> { { 0, 1, 2, 3, 4 } },
            Face<5> { { 0, 5, 18, 6, 1 } },
            Face<5> { { 1, 6, 19, 7, 2 } },
            Face<5> { { 2, 7, 15, 8, 3 } },
            Face<5> { { 3, 8, 16, 9, 4 } },
            Face<5> { { 4, 9, 17, 5, 0 } },
            Face<5> { { 14, 13, 12, 11, 10 } },
            Face<5> { { 11, 16, 8, 15, 10 } },
            Face<5> { { 12, 17, 9, 16, 11 } },
            Face<5> { { 13, 18, 5, 17, 12 } },
            Face<5> { { 14, 19, 6, 18, 13 } },
            Face<5> { { 10, 15, 7, 19, 14 } },
        }
    }.fitDimension(0.5f);
    return DODECAHEDRON;
}

const Solid<3>& icosahedron() {
    static const float N = 1.0f / PHI;
    static const float P = 1.0f;
    static const auto A = vec3(N, P, 0);
    static const auto B = vec3(-N, P, 0);
    static const auto C = vec3(0, N, P);
    static const auto D = vec3(P, 0, N);
    static const auto E = vec3(P, 0, -N);
    static const auto F = vec3(0, N, -P);

    static const Solid<3> ICOSAHEDRON = Solid<3> {
        {
            A, B, C, D, E, F,
            -A, -B, -C, -D, -E, -F,
        },
        FaceVector<3> {
            Face<3> { { 1, 2, 0 } },
            Face<3> { { 2, 3, 0 } }, 
            Face<3> { { 3, 4, 0 } },
            Face<3> { { 4, 5, 0 } },
            Face<3> { { 5, 1, 0 } },

            Face<3> { { 1, 10, 2 } },
            Face<3> { { 11, 2, 10 } },
            Face<3> { { 2, 11, 3 } },
            Face<3> { { 7, 3, 11 } },
            Face<3> { { 3, 7, 4 } },
            Face<3> { { 8, 4, 7 } },
            Face<3> { { 4, 8, 5 } },
            Face<3> { { 9, 5, 8 } },
            Face<3> { { 5, 9, 1 } },
            Face<3> { { 10, 1, 9 } },

            Face<3> { { 8, 7, 6 } },
            Face<3> { { 9, 8, 6 } },
            Face<3> { { 10, 9, 6 } },
            Face<3> { { 11, 10, 6 } },
            Face<3> { { 7, 11, 6 } },
        }
    }.fitDimension(0.5f);
    return ICOSAHEDRON;
}

#if 0
// FIXME solids need per-face vertices, but smooth shaded
// components do not.  Find a way to support using draw elements
// or draw arrays as appropriate
// Maybe special case cone and cylinder since they combine flat
// and smooth shading
void GeometryCache::buildShapes() {
    auto vertexBuffer = std::make_shared<gpu::Buffer>();
    auto indexBuffer = std::make_shared<gpu::Buffer>();
    // Cube 
    setupFlatShape(_shapes[Cube], cube(), _shapeVertices, _shapeIndices);
    // Tetrahedron
    setupFlatShape(_shapes[Tetrahedron], tetrahedron(), _shapeVertices, _shapeIndices);
    // Icosahedron
    setupFlatShape(_shapes[Icosahedron], icosahedron(), _shapeVertices, _shapeIndices);
    // Octahedron
    setupFlatShape(_shapes[Octahedron], octahedron(), _shapeVertices, _shapeIndices);
    // Dodecahedron
    setupFlatShape(_shapes[Dodecahedron], dodecahedron(), _shapeVertices, _shapeIndices);
    
    // Sphere
    // FIXME this uses way more vertices than required.  Should find a way to calculate the indices
    // using shared vertices for better vertex caching
    Solid<3> sphere = icosahedron();
    sphere = tesselate(sphere, ICOSAHEDRON_TO_SPHERE_TESSELATION_COUNT); 
    sphere.fitDimension(1.0f);
    setupSmoothShape(_shapes[Sphere], sphere, _shapeVertices, _shapeIndices);

    // Line
    {
        Index baseVertex = (Index)(_shapeVertices->getSize() / SHAPE_VERTEX_STRIDE);
        ShapeData& shapeData = _shapes[Line];
        shapeData.setupVertices(_shapeVertices, VertexVector{
            vec3(-0.5, 0, 0), vec3(-0.5f, 0, 0),
            vec3(0.5f, 0, 0), vec3(0.5f, 0, 0)
        });
        IndexVector wireIndices;
        // Only two indices
        wireIndices.push_back(0 + baseVertex);
        wireIndices.push_back(1 + baseVertex);
        shapeData.setupIndices(_shapeIndices, IndexVector(), wireIndices);
    }

    // Not implememented yet:

    //Triangle,
    //Quad,
    //Circle,
    //Octahetron,
    //Dodecahedron,
    //Torus,
    //Cone,
    //Cylinder,
}
#endif

gpu::Stream::FormatPointer& getSolidStreamFormat() {
    if (!SOLID_STREAM_FORMAT) {
        SOLID_STREAM_FORMAT = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, POSITION_ELEMENT);
        SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::NORMAL, gpu::Stream::NORMAL, NORMAL_ELEMENT);
    }
    return SOLID_STREAM_FORMAT;
}

gpu::Stream::FormatPointer& getInstancedSolidStreamFormat() {
    if (!INSTANCED_SOLID_STREAM_FORMAT) {
        INSTANCED_SOLID_STREAM_FORMAT = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, POSITION_ELEMENT);
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::NORMAL, gpu::Stream::NORMAL, NORMAL_ELEMENT);
        INSTANCED_SOLID_STREAM_FORMAT->setAttribute(gpu::Stream::COLOR, gpu::Stream::COLOR, COLOR_ELEMENT, 0, gpu::Stream::PER_INSTANCE);
    }
    return INSTANCED_SOLID_STREAM_FORMAT;
}


}
