//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DisplayPlugin.h"

#include <NumericalConstants.h>
#include <ui/Menu.h>

#include "PluginManager.h"

#include "impl/display/NullDisplayPlugin.h"
#include "impl/display/stereo/SideBySideStereoDisplayPlugin.h"
#include "impl/display/stereo/InterleavedStereoDisplayPlugin.h"
#include "impl/display/Basic2DWindowOpenGLDisplayPlugin.h"


const QString& DisplayPlugin::MENU_PATH() {
    static const QString value = "Display";
    return value;
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
DisplayPluginList getDisplayPlugins() {
    DisplayPlugin* PLUGIN_POOL[] = {
        new Basic2DWindowOpenGLDisplayPlugin(),
#ifdef DEBUG
        new NullDisplayPlugin(),
//        new DebugVrDisplayPlugin(),
#endif
        /*
        // Stereo modes
        // SBS left/right
        new SideBySideStereoDisplayPlugin(),
        // Interleaved left/right
        new InterleavedStereoDisplayPlugin(),
        */
        nullptr
    };

    DisplayPluginList result;
    for (int i = 0; PLUGIN_POOL[i]; ++i) {
        DisplayPlugin * plugin = PLUGIN_POOL[i];
        if (plugin->isSupported()) {
            plugin->init();
            result.push_back(DisplayPluginPointer(plugin));
        }
    }
    return result;
}

int64_t DisplayPlugin::getPaintDelayUsecs() const {
    std::lock_guard<std::mutex> lock(_paintDelayMutex);
    return _paintDelayTimer.isValid() ? _paintDelayTimer.nsecsElapsed() / NSECS_PER_USEC : 0;
}

void DisplayPlugin::incrementPresentCount() {
#ifdef DEBUG_PAINT_DELAY
    // Avoid overhead if we are not debugging
    {
        std::lock_guard<std::mutex> lock(_paintDelayMutex);
        _paintDelayTimer.start();
    }
#endif

    ++_presentedFrameIndex;

    // Alert the app that it needs to paint a new presentation frame
    qApp->postEvent(qApp, new QEvent(static_cast<QEvent::Type>(Present)), Qt::HighEventPriority);
}
