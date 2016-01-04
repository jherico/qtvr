//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Plugin.h"

QString Plugin::UNKNOWN_PLUGIN_ID("unknown");

bool Plugin::isSupported() const { return true; }

void Plugin::init() {}

void Plugin::deinit() {}

void Plugin::idle() {}
