//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OculusBaseDisplayPlugin.h"

#include <QtCore/QSharedPointer>

struct SwapFramebufferWrapper;
using SwapFboPtr = QSharedPointer<SwapFramebufferWrapper>;
struct MirrorFramebufferWrapper;
using MirrorFboPtr = QSharedPointer<MirrorFramebufferWrapper>;

class OculusDisplayPlugin : public OculusBaseDisplayPlugin {
    using Parent = OculusBaseDisplayPlugin;
public:
    const QString& getName() const override { return NAME; }

    QString getPreferredAudioInDevice() const override;
    QString getPreferredAudioOutDevice() const override;

protected:
    bool internalActivate() override;
    void hmdPresent() override;
    bool isHmdMounted() const override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void cycleDebugOutput() override;

private:
    static const QString NAME;
    bool _enablePreview { false };
    bool _monoPreview { true };

    MirrorFboPtr     _mirrorFbo;
    SwapFboPtr       _sceneFbo;
    SwapFboPtr       _overlayFbo;
    ovrLayerHeader*  _layers[2];
};

