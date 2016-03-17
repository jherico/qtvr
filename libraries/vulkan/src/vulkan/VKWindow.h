//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <mutex>
#include <QtGui/QWindow>
#include "Config.h"
#include "GLMHelpers.h"

#define FENCE_TIMEOUT 100000000
#define NUM_DESCRIPTOR_SETS 1
/* Number of viewports and number of scissors have to be the same */
/* at pipeline creation and in any call to set them dynamically   */
/* They also have to be the same as each other                    */
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS
/* Number of samples needs to be the same at image creation,      */
/* renderpass creation and pipeline creation.                     */
#define NUM_SAMPLES vk::SampleCountFlagBits::e1


struct swap_chain_buffer {
    vk::Image image;
    vk::ImageView view;
};

struct vertex_buffer_info {
    vk::Buffer buf { nullptr };
    vk::DeviceMemory mem { nullptr };
    vk::DescriptorBufferInfo buffer_info;
};

struct depth_buffer {
    vk::Format format { vk::Format::eD16Unorm };
    vk::Image image { nullptr };
    vk::DeviceMemory mem { nullptr };
    vk::ImageView view { nullptr };
};

struct uniform_buffer_info {
    vk::Buffer buf { nullptr };
    vk::DeviceMemory mem { nullptr };
    vk::DeviceSize size;
    vk::DescriptorBufferInfo buffer_info;
};

namespace vk {
    struct Layer {
        vk::LayerProperties properties;
        std::vector<vk::ExtensionProperties> extensionProperties;
    };

    using Layers = std::map<std::string, Layer>;

    inline Layers initInstanceLayers() {
        Layers result;
        std::vector<vk::LayerProperties> layersProperties;
        vk::Result res = vk::enumerateInstanceLayerProperties(layersProperties);
        assert(vk::Result::eSuccess == res);
        for (const auto& layerProperties : layersProperties) {
            std::string layerName = layerProperties.layerName();
            auto& layer = result[layerName];
            layer.properties = layerProperties;
            res = vk::enumerateInstanceExtensionProperties(layerName, layer.extensionProperties);
        }
        return result;
    }

    inline Layers initPhysicalDeviceLayers(PhysicalDevice& physicalDevice) {
        Layers result;
        std::vector<vk::LayerProperties> layersProperties;
        vk::Result res = physicalDevice.enumerateDeviceLayerProperties(layersProperties);
        assert(vk::Result::eSuccess == res);
        for (const auto& layerProperties : layersProperties) {
            std::string layerName = layerProperties.layerName();
            auto& layer = result[layerName];
            layer.properties = layerProperties;
            res = physicalDevice.enumerateDeviceExtensionProperties(layerName, layer.extensionProperties);
        }
        return result;
    }
}

class VKWindow : public QWindow {
    Q_OBJECT
public:
    VKWindow(QObject* parent = nullptr);
    virtual ~VKWindow();
    void draw();

signals:
    void aboutToClose();

private:
    friend class VkCloseEventFilter;
    void emitClosing();

    vk::Instance instance;
    vk::Layers instanceLayers;
    vk::PhysicalDevice physicalDevice;
    vk::Layers physicalDeviceLayers;
    vk::CommandPool cmd_pool;
    vk::Semaphore presentCompleteSemaphore;
    vk::SwapchainKHR swap_chain;
    vk::SurfaceKHR surface;
    vk::Device device;
    vk::Queue queue;
    std::vector<vk::CommandBuffer> cmd_buffers; // Buffer for initialization commands
    vk::CommandBuffer cmd; // Buffer for initialization commands
    vk::PipelineCache pipelineCache;
    vk::RenderPass render_pass;
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
    vk::Format format { vk::Format::eUndefined };
    uint32_t graphics_queue_family_index { UINT32_MAX };
    uint32_t swapchainImageCount { UINT32_MAX };
    vertex_buffer_info vertex_buffer;
    uniform_buffer_info uniform_data;
    bool use_staging_buffer;

    depth_buffer depth;
    vk::PhysicalDeviceProperties gpu_props;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Extent2D size;
//    int width { -1 }, height { -1 };
    std::vector<swap_chain_buffer> buffers;

    vk::DeviceMemory stagingMemory { nullptr };
    vk::Image stagingImage { nullptr };
    vk::VertexInputBindingDescription vi_binding;
    vk::VertexInputAttributeDescription vi_attribs[2];

    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 MVP;


    std::vector<vk::DescriptorSetLayout> desc_layouts;
    std::array<vk::ShaderModule, 2> shaderModules;

    vk::DescriptorPool desc_pool;
    std::vector<vk::DescriptorSet> desc_set;

//    PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
//    PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
//    PFN_vkDebugReportMessageEXT dbgBreakCallback;
//    std::vector<VkDebugReportCallbackEXT> debug_report_callbacks;

    uint32_t current_buffer { UINT32_MAX };

    vk::Viewport viewport;
    vk::Rect2D scissor;

    void set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout old_image_layout, vk::ImageLayout new_image_layout);
    bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
    bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask_, vk::MemoryAllocateInfo& allocInfo);

    //vk::Result init_global_extension_properties(layer_properties &layer_props);
    //vk::Result init_global_layer_properties();
    //vk::Result init_device_extension_properties(layer_properties &layer_props);
    //vk::Result init_device_layer_properties();

    void init_instance(char const *const app_short_name);
    void init_physical_device();
    void init_device();
    void init_queue_family_index();

    void destroy_debug_report_callback();
    void destroy_window();
    void init_window_size(int32_t default_width, int32_t default_height);
    void init_depth_buffer();
    void init_swapchain_extension();
    void init_presentable_image();
    void execute_queue_cmdbuf(const vk::CommandBuffer& cmd_bufs);
    void execute_pre_present_barrier();
    void execute_present_image();
    void init_swap_chain();
    void init_uniform_buffer();
    void update_uniform_buffer();
    void init_descriptor_and_pipeline_layouts(bool use_texture);
    void init_renderpass(bool clear = true);
    void init_framebuffers();
    void init_command_pool();
    void init_command_buffer();
    void execute_queue_command_buffer();
    void init_device_queue();
    void init_vertex_buffer(const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture);
    void init_descriptor_pool(bool use_texture);
    void init_descriptor_set(bool use_texture);
    void init_shaders(const char *vertShaderText, const char *fragShaderText);
    void init_pipeline_cache();
    void init_pipeline(VkBool32 include_vi = true);
    void init_sampler(vk::Sampler &sampler);
    void init_texture(const char *textureName);
    void init_viewports();
    void init_scissors();
    void init_fence(vk::Fence &fence);
    void init_submit_info(vk::SubmitInfo &submit_info, vk::PipelineStageFlags &pipe_stage_flags);
    void init_present_info(vk::PresentInfoKHR &present);
    void destroy_pipeline();
    void destroy_pipeline_cache();
    void destroy_uniform_buffer();
    void destroy_descriptor_and_pipeline_layouts();
    void destroy_descriptor_pool();
    void destroy_shaders();
    void destroy_command_buffer();
    void destroy_command_pool();
    void destroy_depth_buffer();
    void destroy_vertex_buffer();
    void destroy_swap_chain();
    void destroy_framebuffers();
    void destroy_renderpass();
    void destroy_device();
    void destroy_instance();
    void destroy_textures();
};



