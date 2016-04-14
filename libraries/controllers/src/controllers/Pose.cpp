//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QtQml/QJSValue>

#include "Pose.h"

namespace controller {

    Pose::Pose(const vec3& translation, const quat& rotation,
            const vec3& velocity, const vec3& angularVelocity) :
            translation(translation), rotation(rotation), velocity(velocity), angularVelocity(angularVelocity), valid (true) { }

    bool Pose::operator==(const Pose& right) const {
        // invalid poses return false for comparison, even against identical invalid poses, like NaN
        if (!valid || !right.valid) {
            return false;
        }

        // FIXME add margin of error?  Or add an additional withinEpsilon function?
        return translation == right.getTranslation() && rotation == right.getRotation() &&
            velocity == right.getVelocity() && angularVelocity == right.getAngularVelocity();
    }

    Pose Pose::transform(const glm::mat4& mat) const {
        auto rot = glmExtractRotation(mat);
        Pose pose(transformPoint(mat, translation),
                  rot * rotation,
                  transformVectorFast(mat, velocity),
                  rot * angularVelocity);
        pose.valid = valid;
        return pose;
    }
}

