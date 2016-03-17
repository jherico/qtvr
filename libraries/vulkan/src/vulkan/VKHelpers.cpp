#include "VKHelpers.h"

#include <QtCore/QDebug>

#include <SPIRV/GlslangToSpv.h>
#include <GLMHelpers.h>

const vk::Optional<const vk::AllocationCallbacks> VK::ALLOC = vk::AllocationCallbacks::null();

VkBool32 VKAPI_CALL debugReport(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData) {
    qDebug() << "Layer" << pLayerPrefix << " Message " << pMessage;
    return VK_TRUE;
}

#ifdef VK_NO_PROTOTYPES

PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

void initDebugReport(const vk::Instance& instance) {
    vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
}

#else 

PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance                                  instance,
    const VkDebugReportCallbackCreateInfoEXT*   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDebugReportCallbackEXT*                   pCallback) {
    return dbgCreateDebugReportCallback(instance, pCreateInfo, pAllocator, pCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance                                  instance,
    VkDebugReportCallbackEXT                    callback,
    const VkAllocationCallbacks*                pAllocator) {
    dbgDestroyDebugReportCallback(instance, callback, pAllocator);
}

void VK::initDebugReport(const vk::Instance& instance) {
    dbgCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT");
    dbgDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT");

    static const vk::DebugReportFlagsEXT flags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning);
    instance.createDebugReportCallbackEXT(vk::DebugReportCallbackCreateInfoEXT(flags, debugReport, nullptr), VK::ALLOC);
}

#endif

void init_resources(TBuiltInResource &Resources) {
    Resources.maxLights = 32;
    Resources.maxClipPlanes = 6;
    Resources.maxTextureUnits = 32;
    Resources.maxTextureCoords = 32;
    Resources.maxVertexAttribs = 64;
    Resources.maxVertexUniformComponents = 4096;
    Resources.maxVaryingFloats = 64;
    Resources.maxVertexTextureImageUnits = 32;
    Resources.maxCombinedTextureImageUnits = 80;
    Resources.maxTextureImageUnits = 32;
    Resources.maxFragmentUniformComponents = 4096;
    Resources.maxDrawBuffers = 32;
    Resources.maxVertexUniformVectors = 128;
    Resources.maxVaryingVectors = 8;
    Resources.maxFragmentUniformVectors = 16;
    Resources.maxVertexOutputVectors = 16;
    Resources.maxFragmentInputVectors = 15;
    Resources.minProgramTexelOffset = -8;
    Resources.maxProgramTexelOffset = 7;
    Resources.maxClipDistances = 8;
    Resources.maxComputeWorkGroupCountX = 65535;
    Resources.maxComputeWorkGroupCountY = 65535;
    Resources.maxComputeWorkGroupCountZ = 65535;
    Resources.maxComputeWorkGroupSizeX = 1024;
    Resources.maxComputeWorkGroupSizeY = 1024;
    Resources.maxComputeWorkGroupSizeZ = 64;
    Resources.maxComputeUniformComponents = 1024;
    Resources.maxComputeTextureImageUnits = 16;
    Resources.maxComputeImageUniforms = 8;
    Resources.maxComputeAtomicCounters = 8;
    Resources.maxComputeAtomicCounterBuffers = 1;
    Resources.maxVaryingComponents = 60;
    Resources.maxVertexOutputComponents = 64;
    Resources.maxGeometryInputComponents = 64;
    Resources.maxGeometryOutputComponents = 128;
    Resources.maxFragmentInputComponents = 128;
    Resources.maxImageUnits = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    Resources.maxCombinedShaderOutputResources = 8;
    Resources.maxImageSamples = 0;
    Resources.maxVertexImageUniforms = 0;
    Resources.maxTessControlImageUniforms = 0;
    Resources.maxTessEvaluationImageUniforms = 0;
    Resources.maxGeometryImageUniforms = 0;
    Resources.maxFragmentImageUniforms = 8;
    Resources.maxCombinedImageUniforms = 8;
    Resources.maxGeometryTextureImageUnits = 16;
    Resources.maxGeometryOutputVertices = 256;
    Resources.maxGeometryTotalOutputComponents = 1024;
    Resources.maxGeometryUniformComponents = 1024;
    Resources.maxGeometryVaryingComponents = 64;
    Resources.maxTessControlInputComponents = 128;
    Resources.maxTessControlOutputComponents = 128;
    Resources.maxTessControlTextureImageUnits = 16;
    Resources.maxTessControlUniformComponents = 1024;
    Resources.maxTessControlTotalOutputComponents = 4096;
    Resources.maxTessEvaluationInputComponents = 128;
    Resources.maxTessEvaluationOutputComponents = 128;
    Resources.maxTessEvaluationTextureImageUnits = 16;
    Resources.maxTessEvaluationUniformComponents = 1024;
    Resources.maxTessPatchComponents = 120;
    Resources.maxPatchVertices = 32;
    Resources.maxTessGenLevel = 64;
    Resources.maxViewports = 16;
    Resources.maxVertexAtomicCounters = 0;
    Resources.maxTessControlAtomicCounters = 0;
    Resources.maxTessEvaluationAtomicCounters = 0;
    Resources.maxGeometryAtomicCounters = 0;
    Resources.maxFragmentAtomicCounters = 8;
    Resources.maxCombinedAtomicCounters = 8;
    Resources.maxAtomicCounterBindings = 1;
    Resources.maxVertexAtomicCounterBuffers = 0;
    Resources.maxTessControlAtomicCounterBuffers = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers = 0;
    Resources.maxGeometryAtomicCounterBuffers = 0;
    Resources.maxFragmentAtomicCounterBuffers = 1;
    Resources.maxCombinedAtomicCounterBuffers = 1;
    Resources.maxAtomicCounterBufferSize = 16384;
    Resources.maxTransformFeedbackBuffers = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances = 8;
    Resources.maxCombinedClipAndCullDistances = 8;
    Resources.maxSamples = 4;
    Resources.limits.nonInductiveForLoops = 1;
    Resources.limits.whileLoops = 1;
    Resources.limits.doWhileLoops = 1;
    Resources.limits.generalUniformIndexing = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing = 1;
    Resources.limits.generalSamplerIndexing = 1;
    Resources.limits.generalVariableIndexing = 1;
    Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

EShLanguage FindLanguage(const vk::ShaderStageFlagBits shader_type) {
    switch (shader_type) {
    case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
    case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
    case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
    case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
    case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
    case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
    default: return EShLangVertex;
    }
}

void VK::initGlsl() {
    glslang::InitializeProcess();
}

void VK::finalizeGlsl() {
    glslang::FinalizeProcess();
}

//
// Compile a given string containing GLSL into SPV for use by VK
//
std::vector<uint32_t> VK::glslToSpv(const vk::ShaderStageFlagBits shaderType, const std::string& shaderSource) {
    std::vector<uint32_t> result;
    TBuiltInResource Resources;
    init_resources(Resources);

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    EShLanguage stage = FindLanguage(shaderType);
    glslang::TShader *shader = new glslang::TShader(stage);
    {
        const char *shaderStrings[1] = { shaderSource.c_str() };
        shader->setStrings(shaderStrings, 1);
        if (!shader->parse(&Resources, 100, false, messages)) {
            throw new std::runtime_error(shader->getInfoLog());
        }
    }

    glslang::TProgram &program = *new glslang::TProgram;
    program.addShader(shader);
    if (!program.link(messages)) {
        throw new std::runtime_error(shader->getInfoLog());
    }
    glslang::GlslangToSpv(*program.getIntermediate(stage), result);
    return result;
}

vk::ShaderModule VK::glslToShaderModule(const vk::Device& device, const vk::ShaderStageFlagBits shaderType, const std::string& shaderSource) {
    std::vector<uint32_t> spv = VK::glslToSpv(shaderType, shaderSource);
    vk::ShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo
        .codeSize(spv.size() * sizeof(uint32_t))
        .pCode(spv.data());
    return device.createShaderModule(moduleCreateInfo, ALLOC);
}

using SwapImage = std::pair<vk::Image, vk::ImageView>;
using SwapImages = std::vector<SwapImage>;

class SwapChain {

public:
    void init(vk::SurfaceKHR surface, vk::Format format, vk::Extent2D size) {
        _createInfo
            .surface(surface)
            .imageFormat(format)
            .clipped(true)
            .imageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .imageArrayLayers(1);

        vk::Result res;
        vk::SurfaceCapabilitiesKHR surfCapabilities;
        res = _physicalDevice.getSurfaceCapabilitiesKHR(surface, surfCapabilities);

        {
            // If mailbox mode is available, use it, as is the lowest-latency non-
            // tearing mode.  If not, try IMMEDIATE which will usually be available,
            // and is fastest (though it tears).  If not, fall back to FIFO which is
            // always available.
            std::set<vk::PresentModeKHR> presentModes;
            {
                std::vector<vk::PresentModeKHR> presentModesVec;
                res = _physicalDevice.getSurfacePresentModesKHR(surface, presentModesVec);
                presentModes.insert(presentModesVec.begin(), presentModesVec.end());
            }
            assert(res == vk::Result::eSuccess);
            if (presentModes.count(vk::PresentModeKHR::eMailboxKHR)) {
                _createInfo.presentMode(vk::PresentModeKHR::eMailboxKHR);
            } else if (presentModes.count(vk::PresentModeKHR::eImmediateKHR)) {
                _createInfo.presentMode(vk::PresentModeKHR::eImmediateKHR);
            } else {
                _createInfo.presentMode(vk::PresentModeKHR::eFifoKHR);
            }
        }

        {
            assert(res == vk::Result::eSuccess);
            // width and height are either both -1, or both not -1.
            if (surfCapabilities.currentExtent().width() == (uint32_t)-1) {
                // If the surface size is undefined, the size is set to the size of the images requested.
                _createInfo.imageExtent(size);
            } else {
                // If the surface size is defined, the swap chain size must match
                _createInfo.imageExtent(surfCapabilities.currentExtent());
            }

            // Determine the number of VkImage's to use in the swap chain (we desire to
            // own only 1 image at a time, besides the images being displayed and
            // queued for display):
            _createInfo.minImageCount(std::min(surfCapabilities.maxImageCount(), surfCapabilities.minImageCount() + 1));
            if (!(vk::SurfaceTransformFlagBitsKHR::eIdentity & surfCapabilities.supportedTransforms())) {
                _createInfo.preTransform(surfCapabilities.currentTransform());
            }
        }
    }


    void init(const vk::Extent2D& size) {
        vk::Device device;
        _swapChain = device.createSwapchainKHR(_createInfo, VK::ALLOC);
        assert(res == vk::Result::eSuccess);
        
        std::vector<vk::Image> images;
        res = device.getSwapchainImagesKHR(_swapChain, images);
        assert(res == vk::Result::eSuccess);
        _images.resize(images.size());
        for (uint32_t i = 0; i < _images.size(); i++) {
            SwapImage& swapImage = _images[i];
            swapImage.first = images[i];

            vk::ImageSubresourceRange range;
            range.aspectMask(vk::ImageAspectFlagBits::eColor).layerCount(1).levelCount(1);

            vk::ImageMemoryBarrier image_memory_barrier;
            image_memory_barrier
                .oldLayout(vk::ImageLayout::eUndefined)
                .newLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .image(swapImage.first)
                .subresourceRange(range)
                .dstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, { image_memory_barrier });

            vk::ImageViewCreateInfo color_image_view;
            color_image_view
                .format(_createInfo.imageFormat())
                .image(swapImage.first)
                .viewType(vk::ImageViewType::e2D)
                .subresourceRange(range);
            swapImage.second = device.createImageView(color_image_view, VK::ALLOC);
        }
    }
private:
    uint32_t _current { 0 };
    vk::Device _device;
    vk::PhysicalDevice _physicalDevice;
    vk::SwapchainKHR _swapChain;
    vk::SwapchainCreateInfoKHR _createInfo;
    SwapImages _images;
};


