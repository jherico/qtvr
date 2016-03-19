//
//  Created by Bradley Austin Davis on 2015/11/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrHelpers.h"

#include <atomic>
#include <mutex>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>


Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.plugins.display")

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static int refCount { 0 };
static Mutex mutex;
static vr::IVRSystem* activeHmd { nullptr };

static const uint32_t RELEASE_OPENVR_HMD_DELAY_MS = 5000;

vr::IVRSystem* acquireOpenVrSystem() {
    bool hmdPresent = vr::VR_IsHmdPresent();
    if (hmdPresent) {
        Lock lock(mutex);
        if (!activeHmd) {
            qCDebug(displayplugins) << "openvr: No vr::IVRSystem instance active, building";
            vr::EVRInitError eError = vr::VRInitError_None;
            activeHmd = vr::VR_Init(&eError);
            qCDebug(displayplugins) << "openvr display: HMD is " << activeHmd << " error is " << eError;
        }
        if (activeHmd) {
            qCDebug(displayplugins) << "openvr: incrementing refcount";
            ++refCount;
        }
    } else {
        qCDebug(displayplugins) << "openvr: no hmd present";
    }
    return activeHmd;
}

void releaseOpenVrSystem() {
    if (activeHmd) {
        Lock lock(mutex);
        qCDebug(displayplugins) << "openvr: decrementing refcount";
        --refCount;
        if (0 == refCount) {
            qCDebug(displayplugins) << "openvr: zero refcount, deallocate VR system";
            vr::VR_Shutdown();
        }
    }
}
