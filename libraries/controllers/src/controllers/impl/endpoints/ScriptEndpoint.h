//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_ScriptEndpoint_h
#define hifi_Controllers_ScriptEndpoint_h

#include <QtQml/QJSValue>

#include "../Endpoint.h"

namespace controller {

class ScriptEndpoint : public Endpoint {
    Q_OBJECT;
public:
    using Endpoint::apply;
    ScriptEndpoint(const QJSValue& callable)
        : Endpoint(Input::INVALID_INPUT), _callable(callable) {
    }

    virtual float peek() const override;
    virtual void apply(float newValue, const Pointer& source) override;


    virtual Pose peekPose() const override;
    virtual void apply(const Pose& newValue, const Pointer& source) override;

    virtual bool isPose() const override { return _returnPose; }

protected:
    Q_INVOKABLE void updateValue();
    Q_INVOKABLE virtual void internalApply(float newValue, int sourceID);

    Q_INVOKABLE void updatePose();
    Q_INVOKABLE virtual void internalApply(const Pose& newValue, int sourceID);
private:
    QJSValue _callable;
    float _lastValueRead { 0.0f };
    float _lastValueWritten { 0.0f };

    bool _returnPose { false };
    Pose _lastPoseRead;
    Pose _lastPoseWritten;
};

}

#endif
