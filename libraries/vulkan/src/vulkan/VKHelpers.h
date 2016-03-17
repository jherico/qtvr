//
//  Created by Bradley Austin Davis on 2016/03/19
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "Config.h"


class VK {
public:
    static const vk::Optional<const vk::AllocationCallbacks> ALLOC;
    static void initGlsl();
    static void finalizeGlsl();
    static void initDebugReport(const vk::Instance& instance);
    static std::vector<uint32_t> glslToSpv(const vk::ShaderStageFlagBits shaderType, const std::string& shaderSource);
    static vk::ShaderModule glslToShaderModule(const vk::Device& device, const vk::ShaderStageFlagBits shaderType, const std::string& shaderSource);
};