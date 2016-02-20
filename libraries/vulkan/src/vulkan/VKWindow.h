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
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT


struct layer_properties {
    VkLayerProperties properties;
    std::vector<vk::ExtensionProperties> extensions;
};

struct swap_chain_buffer {
    VkImage image;
    VkImageView view;
};

struct vertex_buffer_info {
    vk::Buffer buf { nullptr };
    vk::DeviceMemory mem { nullptr };
    VkDescriptorBufferInfo buffer_info;
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


    vk::Instance inst { nullptr };
    vk::CommandPool cmd_pool { nullptr };
    vk::Semaphore presentCompleteSemaphore { nullptr };
    vk::SwapchainKHR swap_chain { nullptr };
    vk::SurfaceKHR surface { nullptr };
    vk::Device device { nullptr };
    vk::Queue queue { nullptr };
    vk::CommandBuffer cmd { nullptr }; // Buffer for initialization commands
    vk::PipelineCache pipelineCache { nullptr };
    vk::RenderPass render_pass { nullptr };
    vk::Pipeline pipeline { nullptr };
    vk::PipelineLayout pipeline_layout { nullptr };
    vk::Format format { vk::Format::eUndefined };
    uint32_t graphics_queue_family_index { UINT32_MAX };
    uint32_t swapchainImageCount { UINT32_MAX };
    vertex_buffer_info vertex_buffer;
    uniform_buffer_info uniform_data;
    bool use_staging_buffer;

    depth_buffer depth;
    std::vector<const char *> instance_layer_names;
    std::vector<const char *> instance_extension_names;
    std::vector<layer_properties> instance_layer_properties;
    std::vector<VkExtensionProperties> instance_extension_properties;
    std::vector<const char *> device_layer_names;
    std::vector<const char *> device_extension_names;
    std::vector<layer_properties> device_layer_properties;
    std::vector<VkExtensionProperties> device_extension_properties;
    std::vector<VkPhysicalDevice> gpus;
    std::vector<vk::QueueFamilyProperties> queue_props;
    vk::PhysicalDeviceProperties gpu_props;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    std::vector<vk::Framebuffer> framebuffers;
    int width { -1 }, height { -1 };
    std::vector<swap_chain_buffer> buffers;

    vk::DeviceMemory stagingMemory { nullptr };
    vk::Image stagingImage { nullptr };
    vk::VertexInputBindingDescription vi_binding;
    vk::VertexInputAttributeDescription vi_attribs[2];

    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 MVP;


    std::vector<VkDescriptorSetLayout> desc_layout;
    vk::PipelineShaderStageCreateInfo shaderStages[2];

    VkDescriptorPool desc_pool;
    std::vector<VkDescriptorSet> desc_set;

    PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
    PFN_vkDebugReportMessageEXT dbgBreakCallback;
    std::vector<VkDebugReportCallbackEXT> debug_report_callbacks;

    uint32_t current_buffer { UINT32_MAX };
    uint32_t queue_count;

    VkViewport viewport;
    VkRect2D scissor;

    void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout);
    bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
    bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask_, vk::MemoryAllocateInfo& allocInfo);

    vk::Result init_global_extension_properties(layer_properties &layer_props);
    vk::Result init_global_layer_properties();
    vk::Result init_device_extension_properties(layer_properties &layer_props);
    vk::Result init_device_layer_properties();
    VkBool32 demo_check_layers(const std::vector<layer_properties> &layer_props, const std::vector<const char *> &layer_names);
    void init_instance_extension_names();
    vk::Result  init_instance(char const *const app_short_name);
    void init_device_extension_names();
    vk::Result  init_device();
    vk::Result  init_enumerate_device(uint32_t gpu_count = 1);
    void init_queue_family_index();
    vk::Result  init_debug_report_callback(PFN_vkDebugReportCallbackEXT dbgFunc);
    void destroy_debug_report_callback();
    void destroy_window();
    void init_window_size(int32_t default_width, int32_t default_height);
    void init_depth_buffer();
    void init_swapchain_extension();
    void init_presentable_image();
    void execute_queue_cmdbuf(const VkCommandBuffer *cmd_bufs);
    void execute_pre_present_barrier();
    void execute_present_image();
    void init_swap_chain();
    void init_uniform_buffer();
    void update_uniform_buffer();
    void init_descriptor_and_pipeline_layouts(bool use_texture);
    void init_renderpass(bool include_depth, bool clear = true);
    void init_framebuffers(bool include_depth);
    void init_command_pool();
    void init_command_buffer();
    void execute_begin_command_buffer();
    void execute_end_command_buffer();
    void execute_queue_command_buffer();
    void init_device_queue();
    void init_vertex_buffer(const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture);
    void init_descriptor_pool(bool use_texture);
    void init_descriptor_set(bool use_texture);
    void init_shaders(const char *vertShaderText, const char *fragShaderText);
    void init_pipeline_cache();
    void init_pipeline(VkBool32 include_depth, VkBool32 include_vi = true);
    void init_sampler(VkSampler &sampler);
    void init_texture(const char *textureName);
    void init_viewports();
    void init_scissors();
    void init_fence(VkFence &fence);
    void init_submit_info(VkSubmitInfo &submit_info, VkPipelineStageFlags &pipe_stage_flags);
    void init_present_info(VkPresentInfoKHR &present);
    void init_clear_color_and_depth(VkClearValue *clear_values);
    void init_render_pass_begin_info(VkRenderPassBeginInfo &rp_begin);
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

//
//    static void createInstance();
//
//    static VkBool32 debugCallback(
//        VkDebugReportFlagsEXT                       flags,
//        VkDebugReportObjectTypeEXT                  objectType,
//        uint64_t                                    object,
//        size_t                                      location,
//        int32_t                                     messageCode,
//        const char*                                 pLayerPrefix,
//        const char*                                 pMessage,
//        void*                                       pUserData);
//
//
//    std::vector<vk::PhysicalDevice> _gpus;
//    vk::PhysicalDevice _gpu;
//    vk::Device _device { nullptr };
//    std::vector<vk::QueueFamilyProperties> _queueProperties;
//    uint32_t _targetQueueIndex { UINT32_MAX };
//    vk::CommandBuffer _commandBuffer { nullptr }; // Buffer for initialization commands
//    //std::vector<vk::CommandBuffer> _commandBuffers;
//    vk::CommandPool _commandPool { nullptr };
//    vk::SurfaceKHR _surface { nullptr };
//    vk::SwapchainKHR _swapchain { nullptr };
//    vk::Queue _queue { nullptr };
//    vk::Format _format;
//    struct ImageAndView {
//        vk::Image image;
//        vk::ImageView view;
//    };
//    std::vector<ImageAndView> _swapchainImages;
//
//    void initPhysicalDevice();
//    void initSurface();
//    void initSwapChain();
//    void initDevice();
//    void initCommandPool();
//    void initCommandBuffer();
//    void initDepthBuffer();
//    void initUniformBuffer();
//    void initLayouts();
//    void initRenderpass();
//    void initShaders();
//    void initFramebuffers();
//    void initVertexBuffers();
//    void initPipeline();
//    void initDescriptorPool();
//    void initDescriptorSet();
//    void beginCommandBuffer();
//    void endCommandBuffer();
//    void queueCommandBuffer();
//
//
//    bool _prepared { false };
//    static std::vector<vk::LayerProperties> _instanceLayersProperties;
//    static std::vector<vk::ExtensionProperties> _instanceExtensionProperties;
//
//    std::vector<vk::ExtensionProperties> _deviceExtensionProperties;
//    std::vector<vk::LayerProperties> _deviceLayerProperties;
//    vk::Semaphore _presentCompleteSemaphore;
//    vk::PhysicalDeviceProperties _gpuProperties;
//    vk::PhysicalDeviceMemoryProperties _memoryProperties;
//    std::vector<vk::Framebuffer> _framebuffers;
//    size_t _currentFramebuffer { 0 };
////    std::vector<swap_chain_buffer> buffers;
//
//    struct {
//        vk::Format format;
//        vk::Image image;
//        vk::DeviceMemory mem;
//        vk::ImageView view;
//    } _depth;
//
//    struct {
//        vk::Buffer buf;
//        vk::DeviceMemory mem;
//        vk::DescriptorBufferInfo info;
//    } _uniformData;
//
//    vk::DeviceMemory stagingMemory;
//    vk::Image stagingImage;
//
//    struct {
//        vk::Buffer buf;
//        vk::DeviceMemory mem;
//        vk::DescriptorBufferInfo info;
//    } _vertexBuffer;
//
//    vk::VertexInputBindingDescription _binding;
//    std::array<vk::VertexInputAttributeDescription, 2> _attribs;
//
//    glm::mat4 Projection;
//    glm::mat4 View;
//    glm::mat4 Model;
//    glm::mat4 MVP;
//
//    vk::PipelineLayout _pipelineLayout;
//    std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
//    vk::PipelineCache _pipelineCache;
//    vk::RenderPass _renderPass;
//    vk::Pipeline _pipeline;
//
//    vk::PipelineShaderStageCreateInfo _shaderStages[2];
//    vk::DescriptorPool _descriptorPool;
//    std::array<vk::DescriptorSet, NUM_DESCRIPTOR_SETS> _descriptorSets;
//
//    PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
//    PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
//    PFN_vkDebugReportMessageEXT dbgBreakCallback;
//    std::vector<VkDebugReportCallbackEXT> debug_report_callbacks;
//
//    uint32_t current_buffer;
//    uint32_t queue_count;
//    vk::Viewport _viewport;
//    vk::Rect2D _scissor;
//    vk::Extent2D _extent;
};



