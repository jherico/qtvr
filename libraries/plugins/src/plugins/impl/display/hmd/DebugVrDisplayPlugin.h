//
//  Created by Bradley Austin Davis on 2016/02/07
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "HmdDisplayPlugin.h"

class DebugVrDisplayPlugin : public HmdDisplayPlugin {
public:
    DebugVrDisplayPlugin();
    virtual bool isSupported() const override;

    virtual const QString & getName() const override;
protected:
    uvec2 _desiredFramebufferSize;
};
