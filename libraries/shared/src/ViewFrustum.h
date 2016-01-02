//
//  ViewFrustum.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Simple view frustum class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ViewFrustum_h
#define hifi_ViewFrustum_h

#include "GLMHelpers.h"
#include "Transform.h"
#include "AABox.h"
#include "AACube.h"
#include "Plane.h"

const int TREE_SCALE = 1 << 15; // ~20 miles.. This is the number of meters of the 0.0 to 1.0 voxel universe
const int HALF_TREE_SCALE = 1 << 14;
const float DEFAULT_KEYHOLE_RADIUS = 3.0f;
const float DEFAULT_FIELD_OF_VIEW_DEGREES = 45.0f;
const float DEFAULT_ASPECT_RATIO = 16.0f / 9.0f;
const float DEFAULT_NEAR_CLIP = 0.08f;
const float DEFAULT_FAR_CLIP = (float)HALF_TREE_SCALE;

class ViewFrustum {
public:
    // setters for camera attributes
    void setPosition(const vec3& position);
    void setOrientation(const quat& orientation);

    // getters for camera attributes
    const vec3& getPosition() const { return _position; }
    const quat& getOrientation() const { return _orientation; }
    const vec3& getDirection() const { return _direction; }
    const vec3& getUp() const { return _up; }
    const vec3& getRight() const { return _right; }

    // setters for lens attributes
    void setProjection(const mat4 & projection);
    void getFocalLength(float focalLength) { _focalLength = focalLength; }

    // getters for lens attributes
    const mat4& getProjection() const { return _projection; }
    const mat4& getView() const { return _view; }
    float getWidth() const { return _width; }
    float getHeight() const { return _height; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _nearClip; }
    float getFarClip() const { return _farClip; }
    float getFocalLength() const { return _focalLength; }

    const vec3& getFarTopLeft() const { return _cornersWorld[TOP_LEFT_FAR]; }
    const vec3& getFarTopRight() const { return _cornersWorld[TOP_RIGHT_FAR]; }
    const vec3& getFarBottomLeft() const { return _cornersWorld[BOTTOM_LEFT_FAR]; }
    const vec3& getFarBottomRight() const { return _cornersWorld[BOTTOM_RIGHT_FAR]; }

    const vec3& getNearTopLeft() const { return _cornersWorld[TOP_LEFT_NEAR]; }
    const vec3& getNearTopRight() const { return _cornersWorld[TOP_RIGHT_NEAR]; }
    const vec3& getNearBottomLeft() const { return _cornersWorld[BOTTOM_LEFT_NEAR]; }
    const vec3& getNearBottomRight() const { return _cornersWorld[BOTTOM_RIGHT_NEAR]; }

    // get/set for keyhole attribute
    void  setKeyholeRadius(float keyholdRadius) { _keyholeRadius = keyholdRadius; }
    float getKeyholeRadius() const { return _keyholeRadius; }

    void calculate();

    typedef enum {OUTSIDE, INTERSECT, INSIDE} location;

    ViewFrustum::location pointInFrustum(const vec3& point, bool ignoreKeyhole = false) const;
    ViewFrustum::location sphereInFrustum(const vec3& center, float radius) const;
    ViewFrustum::location cubeInFrustum(const AACube& cube) const;
    ViewFrustum::location boxInFrustum(const AABox& box) const;

    // some frustum comparisons
    bool matches(const ViewFrustum& compareTo, bool debug = false) const;
    bool matches(const ViewFrustum* compareTo, bool debug = false) const { return matches(*compareTo, debug); }

    bool isVerySimilar(const ViewFrustum& compareTo, bool debug = false) const;
    bool isVerySimilar(const ViewFrustum* compareTo, bool debug = false) const { return isVerySimilar(*compareTo, debug); }

    void computePickRay(float x, float y, vec3& origin, vec3& direction) const;

    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                               vec4& nearClipPlane, vec4& farClipPlane) const;

    void printDebugDetails() const;

    vec2 projectPoint(vec3 point, bool& pointInView) const;
    void getFurthestPointFromCamera(const AACube& box, vec3& furthestPoint) const;
    
    float distanceToCamera(const vec3& point) const;
    
    void evalProjectionMatrix(mat4& proj) const;
    void evalViewTransform(Transform& view) const;

private:
    // Used for keyhole calculations
    ViewFrustum::location pointInKeyhole(const vec3& point) const;
    ViewFrustum::location sphereInKeyhole(const vec3& center, float radius) const;
    ViewFrustum::location cubeInKeyhole(const AACube& cube) const;
    ViewFrustum::location boxInKeyhole(const AABox& box) const;

    // camera location/orientation attributes
    vec3 _position; // the position in world-frame
    quat _orientation;
    mat4 _view;

    // Lens attributes
    mat4 _projection;

    // calculated for orientation
    vec3 _direction = IDENTITY_FRONT;
    vec3 _up = IDENTITY_UP;
    vec3 _right = IDENTITY_RIGHT;

    // keyhole attributes
    float _keyholeRadius = DEFAULT_KEYHOLE_RADIUS;
    AACube _keyholeBoundingCube;

    // Calculated values
    mat4 _inverseProjection;
    float _width = 1.0f;
    float _height = 1.0f;
    float _aspectRatio = 1.0f;
    float _nearClip = DEFAULT_NEAR_CLIP;
    float _farClip = DEFAULT_FAR_CLIP;
    float _focalLength = 0.25f;
    float _fieldOfView = DEFAULT_FIELD_OF_VIEW_DEGREES;
    vec4 _corners[8];
    vec3 _cornersWorld[8];
    enum { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE };
    ::Plane _planes[6]; // How will this be used?

    const char* debugPlaneName (int plane) const;

    // Used to project points
    mat4 _ourModelViewProjectionMatrix;
};


#endif // hifi_ViewFrustum_h
