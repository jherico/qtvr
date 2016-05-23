//
//  GeometryCache.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GeometryCache_h
#define hifi_GeometryCache_h

#include <array>
#include <vector>
#include <unordered_set>

#include <GLMHelpers.h>

#include <gpu/Forward.h>
#include <gpu/Resource.h>


namespace geometry {
    using Vertex = glm::vec3;
    using Index = uint16_t;
    using VertexVector = std::vector<Vertex>;
    using IndexVector = std::vector<Index>;

    using IndexPair = uint64_t;
    using IndexPairs = std::unordered_set<IndexPair>;
    IndexPair indexPair(Index a, Index b);

    template <size_t N>
    using Face = std::array<Index, N>;

    template <size_t N>
    using FaceVector = std::vector<Face<N>>;

    template <size_t N>
    struct Solid {
        VertexVector vertices;
        FaceVector<N> faces;

        Solid<N>& fitDimension(float newMaxDimension) {
            float maxDimension = 0;
            for (const auto& vertex : vertices) {
                maxDimension = std::max(maxDimension, std::max(std::max(vertex.x, vertex.y), vertex.z));
            }
            float multiplier = newMaxDimension / maxDimension;
            for (auto& vertex : vertices) {
                vertex *= multiplier;
            }
            return *this;
        }

        vec3 getFaceNormal(size_t faceIndex) const {
            vec3 result;
            const auto& face = faces[faceIndex];
            for (size_t i = 0; i < N; ++i) {
                result += vertices[face[i]];
            }
            result /= N;
            return glm::normalize(result);
        }
    };

    template <size_t N>
    static size_t triangulatedFaceTriangleCount() {
        return N - 2;
    }

    template <size_t N>
    static size_t triangulatedFaceIndexCount() {
        return triangulatedFaceTriangleCount<N>() * VERTICES_PER_TRIANGLE;
    }

    enum Shape {
        Line,
        Triangle,
        Quad,
        Circle,
        Cube,
        Sphere,
        Tetrahedron,
        Octahedron,
        Dodecahedron,
        Icosahedron,
        Torus,
        Cone,
        Cylinder,

        NUM_SHAPES,
    };

    const Solid<3>& tetrahedron();
    const Solid<4>& cube();
    const Solid<3>& octahedron();
    const Solid<5>& dodecahedron();
    const Solid<3>& icosahedron();

    struct ShapeData {
        size_t _solidIndexOffset { 0 };
        size_t _solidCount { 0 };
        size_t _wireIndexOffset { 0 };
        size_t _wireCount { 0 };
        size_t _baseVertex { 0 };

        //gpu::BufferView _positionView;
        //gpu::BufferView _normalView;
        static gpu::BufferPointer _indices;
        static gpu::BufferPointer _vertices;

        static void setup();

        //void setupVertices(gpu::BufferPointer& vertexBuffer, const VertexVector& vertices);
        //void setupIndices(gpu::BufferPointer& indexBuffer, const IndexVector& indices, const IndexVector& wireIndices);
        //void setupBatch(gpu::Batch& batch) const;
        //void draw(gpu::Batch& batch) const;
        //void drawWire(gpu::Batch& batch) const;
        //void drawInstances(gpu::Batch& batch, size_t count) const;
        //void drawWireInstances(gpu::Batch& batch, size_t count) const;

    };

    using Shapes = std::array<ShapeData, NUM_SHAPES>;
    const Shapes& shapes();
}

#endif // hifi_GeometryCache_h
