//
//  ViewFrustum.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViewFrustum.h"

#include <algorithm>


#include <QtCore/QDebug>

#include "NumericalConstants.h"

#include "GeometryUtil.h"
#include "GLMHelpers.h"

using namespace std;

void ViewFrustum::setOrientation(const quat& orientationAsQuaternion) {
    _orientation = orientationAsQuaternion;
    _right = orientationAsQuaternion * IDENTITY_RIGHT;
    _up = orientationAsQuaternion * IDENTITY_UP;
    _direction = orientationAsQuaternion * IDENTITY_FRONT;
    _view = glm::translate(mat4(), _position) * glm::mat4_cast(_orientation);
}

void ViewFrustum::setPosition(const vec3& position) {
    _position = position;
    _view = glm::translate(mat4(), _position) * glm::mat4_cast(_orientation);
}

// Order cooresponds to the order defined in the BoxVertex enum.
static const vec4 NDC_VALUES[8] = {
    vec4(-1, -1, -1, 1),
    vec4(1, -1, -1, 1),
    vec4(1, 1, -1, 1),
    vec4(-1, 1, -1, 1),
    vec4(-1, -1, 1, 1),
    vec4(1, -1, 1, 1),
    vec4(1, 1, 1, 1),
    vec4(-1, 1, 1, 1),
};

void ViewFrustum::setProjection(const mat4& projection) {
    _projection = projection;
    _inverseProjection = glm::inverse(projection);

    // compute our dimensions the usual way
    for (int i = 0; i < 8; ++i) {
        _corners[i] = _inverseProjection * NDC_VALUES[i];
        _corners[i] /= _corners[i].w;
    }
    _nearClip = -_corners[BOTTOM_LEFT_NEAR].z;
    _farClip = -_corners[BOTTOM_LEFT_FAR].z;
    _aspectRatio = (_corners[TOP_RIGHT_NEAR].x - _corners[BOTTOM_LEFT_NEAR].x) /
        (_corners[TOP_RIGHT_NEAR].y - _corners[BOTTOM_LEFT_NEAR].y);

    vec4 top = _inverseProjection * vec4(0, 1, -1, 1);
    top /= top.w;
    _fieldOfView = abs(glm::degrees(2.0f * abs(glm::angle(vec3(0, 0, -1), glm::normalize(vec3(top))))));
}

// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position and direction
//
// Notes on how/why this works:
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculate() {

    // find the intersections of the rays through the corners with the clip planes in view space,
    // then transform them to world space
    mat4 worldMatrix = glm::translate(_position) * mat4(mat3(_right, _up, -_direction));
    vec4 v;
    for (int i = 0; i < 8; ++i) {
        v = worldMatrix * _corners[i];
        v /= v.w;
        _cornersWorld[i] = vec3(v);
    }

    // compute the six planes
    // The planes are defined such that the normal points towards the inside of the view frustum.
    // Testing if an object is inside the view frustum is performed by computing on which side of
    // the plane the object resides. This can be done computing the signed distance from the point
    // to the plane. If it is on the side that the normal is pointing, i.e. the signed distance
    // is positive, then it is on the right side of the respective plane. If an object is on the
    // right side of all six planes then the object is inside the frustum.

    // the function set3Points assumes that the points are given in counter clockwise order, assume you
    // are inside the frustum, facing the plane. Start with any point, and go counter clockwise for
    // three consecutive points
    _planes[TOP_PLANE].set3Points(_cornersWorld[TOP_RIGHT_NEAR], _cornersWorld[TOP_LEFT_NEAR], _cornersWorld[TOP_LEFT_FAR]);
    _planes[BOTTOM_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[BOTTOM_RIGHT_FAR]);
    _planes[LEFT_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[BOTTOM_LEFT_FAR], _cornersWorld[TOP_LEFT_FAR]);
    _planes[RIGHT_PLANE].set3Points(_cornersWorld[BOTTOM_RIGHT_FAR], _cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[TOP_RIGHT_FAR]);
    _planes[NEAR_PLANE].set3Points(_cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[TOP_LEFT_NEAR]);
    _planes[FAR_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_FAR], _cornersWorld[BOTTOM_RIGHT_FAR], _cornersWorld[TOP_RIGHT_FAR]);

    // Also calculate our projection matrix in case people want to project points...
    // Projection matrix : Field of View, ratio, display range : near to far
    vec3 lookAt = _position + _direction;
    mat4 view = glm::lookAt(_position, lookAt, _up);

    // Our ModelViewProjection : multiplication of our 3 matrices (note: model is identity, so we can drop it)
    _ourModelViewProjectionMatrix = _projection * view; // Remember, matrix multiplication is the other way around

    // Set up our keyhole bounding box...
    vec3 corner = _position - _keyholeRadius;
    _keyholeBoundingCube = AACube(corner,(_keyholeRadius * 2.0f));
}

//enum { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE };
const char* ViewFrustum::debugPlaneName (int plane) const {
    switch (plane) {
        case TOP_PLANE:    return "Top Plane";
        case BOTTOM_PLANE: return "Bottom Plane";
        case LEFT_PLANE:   return "Left Plane";
        case RIGHT_PLANE:  return "Right Plane";
        case NEAR_PLANE:   return "Near Plane";
        case FAR_PLANE:    return "Far Plane";
    }
    return "Unknown";
}

ViewFrustum::location ViewFrustum::pointInKeyhole(const vec3& point) const {

    ViewFrustum::location result = INTERSECT;

    float distance = glm::distance(point, _position);
    if (distance > _keyholeRadius) {
        result = OUTSIDE;
    } else if (distance < _keyholeRadius) {
        result = INSIDE;
    }

    return result;
}

// To determine if two spheres intersect, simply calculate the distance between the centers of the two spheres.
// If the distance is greater than the sum of the two sphere radii, they don’t intersect. Otherwise they intersect.
// If the distance plus the radius of sphere A is less than the radius of sphere B then, sphere A is inside of sphere B
ViewFrustum::location ViewFrustum::sphereInKeyhole(const vec3& center, float radius) const {
    ViewFrustum::location result = INTERSECT;

    float distance = glm::distance(center, _position);
    if (distance > (radius + _keyholeRadius)) {
        result = OUTSIDE;
    } else if ((distance + radius) < _keyholeRadius) {
        result = INSIDE;
    }

    return result;
}


// A box is inside a sphere if all of its corners are inside the sphere
// A box intersects a sphere if any of its edges (as rays) interesect the sphere
// A box is outside a sphere if none of its edges (as rays) interesect the sphere
ViewFrustum::location ViewFrustum::cubeInKeyhole(const AACube& cube) const {

    // First check to see if the cube is in the bounding cube for the sphere, if it's not, then we can short circuit
    // this and not check with sphere penetration which is more expensive
    if (!_keyholeBoundingCube.contains(cube)) {
        return OUTSIDE;
    }

    vec3 penetration;
    bool intersects = cube.findSpherePenetration(_position, _keyholeRadius, penetration);

    ViewFrustum::location result = OUTSIDE;

    // if the cube intersects the sphere, then it may also be inside... calculate further
    if (intersects) {
        result = INTERSECT;

        // test all the corners, if they are all inside the sphere, the entire cube is in the sphere
        bool allPointsInside = true; // assume the best
        for (int v = BOTTOM_LEFT_NEAR; v < TOP_LEFT_FAR; v++) {
            vec3 vertex = cube.getVertex((BoxVertex)v);
            if (!pointInKeyhole(vertex)) {
                allPointsInside = false;
                break;
            }
        }

        if (allPointsInside) {
            result = INSIDE;
        }
    }

    return result;
}

// A box is inside a sphere if all of its corners are inside the sphere
// A box intersects a sphere if any of its edges (as rays) interesect the sphere
// A box is outside a sphere if none of its edges (as rays) interesect the sphere
ViewFrustum::location ViewFrustum::boxInKeyhole(const AABox& box) const {

    // First check to see if the box is in the bounding box for the sphere, if it's not, then we can short circuit
    // this and not check with sphere penetration which is more expensive
    if (!_keyholeBoundingCube.contains(box)) {
        return OUTSIDE;
    }

    vec3 penetration;
    bool intersects = box.findSpherePenetration(_position, _keyholeRadius, penetration);

    ViewFrustum::location result = OUTSIDE;

    // if the box intersects the sphere, then it may also be inside... calculate further
    if (intersects) {
        result = INTERSECT;

        // test all the corners, if they are all inside the sphere, the entire box is in the sphere
        bool allPointsInside = true; // assume the best
        for (int v = BOTTOM_LEFT_NEAR; v < TOP_LEFT_FAR; v++) {
            vec3 vertex = box.getVertex((BoxVertex)v);
            if (!pointInKeyhole(vertex)) {
                allPointsInside = false;
                break;
            }
        }

        if (allPointsInside) {
            result = INSIDE;
        }
    }

    return result;
}

ViewFrustum::location ViewFrustum::pointInFrustum(const vec3& point, bool ignoreKeyhole) const {
    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (!ignoreKeyhole && _keyholeRadius >= 0.0f) {
        keyholeResult = pointInKeyhole(point);

        if (keyholeResult == INSIDE) {
            return keyholeResult;
        }
    }

    // If we're not known to be INSIDE the keyhole, then check the regular frustum
    for(int i = 0; i < 6; ++i) {
        float distance = _planes[i].distance(point);
        if (distance < 0) {
            return keyholeResult; // escape early will be the value from checking the keyhole
        }
    }
    return regularResult;
}

ViewFrustum::location ViewFrustum::sphereInFrustum(const vec3& center, float radius) const {
    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = sphereInKeyhole(center, radius);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    float distance;
    for(int i=0; i < 6; i++) {
        distance = _planes[i].distance(center);
        if (distance < -radius) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (distance < radius) {
            regularResult =  INTERSECT;
        }
    }

    return regularResult;
}


ViewFrustum::location ViewFrustum::cubeInFrustum(const AACube& cube) const {

    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = cubeInKeyhole(cube);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    // TODO: These calculations are expensive, taking up 80% of our time in this function.
    // This appears to be expensive because we have to test the distance to each plane.
    // One suggested optimization is to first check against the approximated cone. We might
    // also be able to test against the cone to the bounding sphere of the box.
    for(int i=0; i < 6; i++) {
        const vec3& normal = _planes[i].getNormal();
        const vec3& boxVertexP = cube.getVertexP(normal);
        float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

        const vec3& boxVertexN = cube.getVertexN(normal);
        float planeToBoxVertexNDistance = _planes[i].distance(boxVertexN);

        if (planeToBoxVertexPDistance < 0) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (planeToBoxVertexNDistance < 0) {
            regularResult =  INTERSECT;
        }
    }
    return regularResult;
}

ViewFrustum::location ViewFrustum::boxInFrustum(const AABox& box) const {

    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = boxInKeyhole(box);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    // TODO: These calculations are expensive, taking up 80% of our time in this function.
    // This appears to be expensive because we have to test the distance to each plane.
    // One suggested optimization is to first check against the approximated cone. We might
    // also be able to test against the cone to the bounding sphere of the box.
    for(int i=0; i < 6; i++) {
        const vec3& normal = _planes[i].getNormal();
        const vec3& boxVertexP = box.getVertexP(normal);
        float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

        const vec3& boxVertexN = box.getVertexN(normal);
        float planeToBoxVertexNDistance = _planes[i].distance(boxVertexN);

        if (planeToBoxVertexPDistance < 0) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (planeToBoxVertexNDistance < 0) {
            regularResult =  INTERSECT;
        }
    }
    return regularResult;
}

bool testMatches(quat lhs, quat rhs, float epsilon = EPSILON) {
    return (
        fabs(lhs.x - rhs.x) <= epsilon && 
        fabs(lhs.y - rhs.y) <= epsilon && 
        fabs(lhs.z - rhs.z) <= epsilon && 
        fabs(lhs.w - rhs.w) <= epsilon);
}

bool testMatches(vec3 lhs, vec3 rhs, float epsilon = EPSILON) {
    return (
        fabs(lhs.x - rhs.x) <= epsilon && 
        fabs(lhs.y - rhs.y) <= epsilon && 
        fabs(lhs.z - rhs.z) <= epsilon);
}

bool testMatches(float lhs, float rhs, float epsilon = EPSILON) {
    return (fabs(lhs - rhs) <= epsilon);
}

bool ViewFrustum::matches(const ViewFrustum& compareTo, bool debug) const {
    bool result =
           testMatches(compareTo._position, _position) &&
           testMatches(compareTo._direction, _direction) &&
           testMatches(compareTo._up, _up) &&
           testMatches(compareTo._right, _right) &&
           testMatches(compareTo._fieldOfView, _fieldOfView) &&
           testMatches(compareTo._aspectRatio, _aspectRatio) &&
           testMatches(compareTo._nearClip, _nearClip) &&
           testMatches(compareTo._farClip, _farClip) &&
           testMatches(compareTo._focalLength, _focalLength);

    return result;
}

bool ViewFrustum::isVerySimilar(const ViewFrustum& compareTo, bool debug) const {

    //  Compute distance between the two positions
    const float POSITION_SIMILAR_ENOUGH = 5.0f; // 5 meters
    float positionDistance = glm::distance(_position, compareTo._position);

    // Compute the angular distance between the two orientations
    const float ORIENTATION_SIMILAR_ENOUGH = 10.0f; // 10 degrees in any direction
    quat dQOrientation = _orientation * glm::inverse(compareTo._orientation);
    float angleOrientation = compareTo._orientation == _orientation ? 0.0f : glm::degrees(glm::angle(dQOrientation));
    if (isNaN(angleOrientation)) {
        angleOrientation = 0.0f;
    }

    bool result =
        testMatches(0, positionDistance, POSITION_SIMILAR_ENOUGH) &&
        testMatches(0, angleOrientation, ORIENTATION_SIMILAR_ENOUGH) &&
           testMatches(compareTo._fieldOfView, _fieldOfView) &&
           testMatches(compareTo._aspectRatio, _aspectRatio) &&
           testMatches(compareTo._nearClip, _nearClip) &&
           testMatches(compareTo._farClip, _farClip) &&
           testMatches(compareTo._focalLength, _focalLength);

    return result;
}

void ViewFrustum::computePickRay(float x, float y, vec3& origin, vec3& direction) const {
    origin = _cornersWorld[TOP_LEFT_NEAR] + x * (_cornersWorld[TOP_RIGHT_NEAR] - _cornersWorld[TOP_LEFT_NEAR]) + 
        y * (_cornersWorld[BOTTOM_LEFT_NEAR] - _cornersWorld[TOP_LEFT_NEAR]);
    direction = glm::normalize(origin - _position);
}

void ViewFrustum::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                                        vec4& nearClipPlane, vec4& farClipPlane) const {
    // find the minimum and maximum z values, which will be our near and far clip distances
    nearValue = FLT_MAX;
    farValue = -FLT_MAX;
    for (int i = 0; i < 8; i++) {
        nearValue = min(nearValue, -_corners[i].z);
        farValue = max(farValue, -_corners[i].z);
    }

    // make sure the near clip isn't too small to be valid
    const float MIN_NEAR = 0.01f;
    nearValue = max(MIN_NEAR, nearValue);

    // get the near/far normal and use it to find the clip planes
    vec4 normal = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    nearClipPlane = vec4(-normal.x, -normal.y, -normal.z, glm::dot(normal, _corners[0]));
    farClipPlane = vec4(normal.x, normal.y, normal.z, -glm::dot(normal, _corners[4]));

    // compute the focal proportion (zero is near clip, one is far clip)
    float focalProportion = (_focalLength - _nearClip) / (_farClip - _nearClip);

    // get the extents at Z = -near
    left = FLT_MAX;
    right = -FLT_MAX;
    bottom = FLT_MAX;
    top = -FLT_MAX;
    for (int i = 0; i < 4; i++) {
        vec4 corner = glm::mix(_corners[i], _corners[i + 4], focalProportion);
        vec4 intersection = corner * (-nearValue / corner.z);
        left = min(left, intersection.x);
        right = max(right, intersection.x);
        bottom = min(bottom, intersection.y);
        top = max(top, intersection.y);
    }
}

void ViewFrustum::printDebugDetails() const {
}

vec2 ViewFrustum::projectPoint(vec3 point, bool& pointInView) const {

    vec4 pointVec4 = vec4(point, 1.0f);
    vec4 projectedPointVec4 = _ourModelViewProjectionMatrix * pointVec4;
    pointInView = (projectedPointVec4.w > 0.0f); // math! If the w result is negative then the point is behind the viewer

    // what happens with w is 0???
    float x = projectedPointVec4.x / projectedPointVec4.w;
    float y = projectedPointVec4.y / projectedPointVec4.w;
    vec2 projectedPoint(x,y);

    // if the point is out of view we also need to flip the signs of x and y
    if (!pointInView) {
        projectedPoint.x = -x;
        projectedPoint.y = -y;
    }

    return projectedPoint;
}



// Similar strategy to getProjectedPolygon() we use the knowledge of camera position relative to the
// axis-aligned voxels to determine which of the voxels vertices must be the furthest. No need for
// squares and square-roots. Just compares.
void ViewFrustum::getFurthestPointFromCamera(const AACube& box, vec3& furthestPoint) const {
    const vec3& bottomNearRight = box.getCorner();
    float scale = box.getScale();
    float halfScale = scale * 0.5f;

    if (_position.x < bottomNearRight.x + halfScale) {
        // we are to the right of the center, so the left edge is furthest
        furthestPoint.x = bottomNearRight.x + scale;
    } else {
        furthestPoint.x = bottomNearRight.x;
    }

    if (_position.y < bottomNearRight.y + halfScale) {
        // we are below of the center, so the top edge is furthest
        furthestPoint.y = bottomNearRight.y + scale;
    } else {
        furthestPoint.y = bottomNearRight.y;
    }

    if (_position.z < bottomNearRight.z + halfScale) {
        // we are to the near side of the center, so the far side edge is furthest
        furthestPoint.z = bottomNearRight.z + scale;
    } else {
        furthestPoint.z = bottomNearRight.z;
    }
}

float ViewFrustum::distanceToCamera(const vec3& point) const {
    vec3 temp = getPosition() - point;
    float distanceToPoint = sqrtf(glm::dot(temp, temp));
    return distanceToPoint;
}

void ViewFrustum::evalProjectionMatrix(mat4& proj) const {
    proj = _projection;
}

void ViewFrustum::evalViewTransform(Transform& view) const {
    view.setTranslation(getPosition());
    view.setRotation(getOrientation());
}
