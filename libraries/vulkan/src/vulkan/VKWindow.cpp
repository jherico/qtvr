//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VKWindow.h"

#include <set>

#include <QtCore/QCoreApplication>
#include <QtGui/qevent.h>
#include <QtCore/QDebug>

#include "VKHelpers.h"
#include "CubeData.h"


class VkCloseEventFilter : public QObject {
    Q_OBJECT
public:
    VkCloseEventFilter(QObject *parent) : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::Close) {
            VKWindow* window = dynamic_cast<VKWindow*>(obj);
            if (window) {
                qApp->quit();
                return true;
            }

        }
        return QObject::eventFilter(obj, event);
    }
};

void VKWindow::init_instance(char const *const app_short_name) {
    instanceLayers = vk::initInstanceLayers();

    vk::ApplicationInfo app_info;
    app_info
        .pApplicationName(app_short_name)
        .pEngineName(app_short_name)
        .applicationVersion(1)
        .engineVersion(1)
        .apiVersion(VK_MAKE_VERSION(1, 0, 0));


    std::vector<const char *> instance_layer_names;
    std::vector<const char *> instance_extension_names;
    //instance_layer_names.push_back("VK_LAYER_LUNARG_threading");
    instance_layer_names.push_back("VK_LAYER_LUNARG_draw_state");
    instance_layer_names.push_back("VK_LAYER_LUNARG_image");
    instance_layer_names.push_back("VK_LAYER_LUNARG_mem_tracker");
    instance_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
    instance_layer_names.push_back("VK_LAYER_LUNARG_param_checker");

    instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

    vk::InstanceCreateInfo inst_info;
    inst_info
        .pApplicationInfo(&app_info)
        .enabledLayerCount((uint32_t)instance_layer_names.size())
        .enabledExtensionCount((uint32_t)instance_extension_names.size())
        .ppEnabledExtensionNames(instance_extension_names.data())
        .ppEnabledLayerNames(instance_layer_names.data());

    instance = vk::createInstance(inst_info, VK::ALLOC);
    VK::initDebugReport(instance);
}

void VKWindow::init_device() {
    std::vector<const char *> device_layer_names;
    std::vector<const char *> device_extension_names;

    //device_layer_names.push_back("VK_LAYER_LUNARG_threading");
    device_layer_names.push_back("VK_LAYER_LUNARG_draw_state");
    device_layer_names.push_back("VK_LAYER_LUNARG_image");
    device_layer_names.push_back("VK_LAYER_LUNARG_mem_tracker");
    device_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
    device_layer_names.push_back("VK_LAYER_LUNARG_param_checker");
    device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    float queue_priorities[1] = { 0.0 };
    vk::DeviceQueueCreateInfo queue_info;
    queue_info
        .queueCount(1)
        .queueFamilyIndex(graphics_queue_family_index)
        .pQueuePriorities(queue_priorities);

    vk::DeviceCreateInfo device_info;
    device_info.queueCreateInfoCount(1).pQueueCreateInfos(&queue_info);

    if (!device_layer_names.empty()) {
        device_info.enabledLayerCount((uint32_t)device_layer_names.size());
        auto layerNames = device_layer_names.data();
        device_info.ppEnabledLayerNames(layerNames);
    }

    if (!device_extension_names.empty()) {
        device_info.enabledExtensionCount((uint32_t)device_extension_names.size());
        auto extensionNames = device_extension_names.data();
        device_info.ppEnabledExtensionNames(extensionNames);
    }

    device = physicalDevice.createDevice(device_info, VK::ALLOC);
}

void VKWindow::init_physical_device() {
    std::vector<vk::PhysicalDevice> physicalDevices;
    vk::Result res = instance.enumeratePhysicalDevices(physicalDevices);
    assert(res == vk::Result::eSuccess);
    physicalDevice = physicalDevices[0];

    /* This is as good a place as any to do this */
    memory_properties = physicalDevice.getMemoryProperties();
    gpu_props = physicalDevice.getProperties();
}

void VKWindow::destroy_window() {
    instance.destroySurfaceKHR(surface, nullptr);
}

void VKWindow::init_window_size(int32_t default_width, int32_t default_height) {
    size = vk::Extent2D(default_width, default_height);
    setGeometry(100 + 1920, 100, default_width, default_height);
    show();
}

bool VKWindow::memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask_, uint32_t *typeIndex) {
    // Search memtypes to find first index with those properties
    vk::MemoryPropertyFlags requirements_mask = reinterpret_cast<const vk::MemoryPropertyFlags&>(requirements_mask_);
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            auto memoryType = memory_properties.memoryTypes()[i];
            auto propertyFlags = memoryType.propertyFlags();
            auto matched = propertyFlags & requirements_mask;
            // Type is available, does it match user properties?
            if (matched == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

bool VKWindow::memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask_, vk::MemoryAllocateInfo& allocInfo) {
    uint32_t type_index;
    if (memory_type_from_properties(typeBits, requirements_mask_, &type_index)) {
        allocInfo.memoryTypeIndex(type_index);
        return true;
    }
    return false;
}

void VKWindow::init_depth_buffer() {
    bool pass;

    vk::ImageCreateInfo image_info;
    const vk::Format depth_format = depth.format;

    vk::FormatProperties props = physicalDevice.getFormatProperties(depth_format);
    if (vk::FormatFeatureFlagBits::eDepthStencilAttachment & props.linearTilingFeatures()) {
        image_info.tiling(vk::ImageTiling::eLinear);
    } else if (vk::FormatFeatureFlagBits::eDepthStencilAttachment & props.optimalTilingFeatures()) {
        image_info.tiling(vk::ImageTiling::eOptimal);
    } else {
        qFatal("depth_format unsupported.");
    }

    image_info
        .imageType(vk::ImageType::e2D)
        .format(depth_format)
        .extent(vk::Extent3D(size.width(), size.height(), 1))
        .mipLevels(1)
        .arrayLayers(1)
        .usage(vk::ImageUsageFlagBits::eDepthStencilAttachment);

    /* Create image */
    depth.image = device.createImage(image_info, VK::ALLOC);

    vk::MemoryRequirements mem_reqs = device.getImageMemoryRequirements(depth.image);

    vk::MemoryAllocateInfo mem_alloc;
    mem_alloc.allocationSize(mem_reqs.size());
    /* Use the memory properties to determine the type of memory required */
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(), 0, mem_alloc);
    assert(pass);

    /* Allocate memory */
    depth.mem = device.allocateMemory(mem_alloc, VK::ALLOC);

    /* Bind memory */
    device.bindImageMemory(depth.image, depth.mem, 0);
    vk::ImageViewCreateInfo view_info;
    view_info
        .format(depth_format)
        .subresourceRange(vk::ImageSubresourceRange()
        .aspectMask(vk::ImageAspectFlagBits::eDepth)
        .levelCount(1)
        .layerCount(1))
        .viewType(vk::ImageViewType::e2D);

    /* Set the image layout to depth stencil optimal */
    set_image_layout(depth.image, view_info.subresourceRange().aspectMask(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    /* Create image view */
    view_info.image(depth.image);
    depth.view = device.createImageView(view_info, VK::ALLOC);
}

void VKWindow::set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout old_image_layout, vk::ImageLayout new_image_layout) {
    /* DEPENDS on cmd and queue initialized */

    assert(cmd);
    assert(queue);
    vk::ImageMemoryBarrier image_memory_barrier;
    image_memory_barrier
        .oldLayout(old_image_layout)
        .newLayout(new_image_layout)
        .image(image)
        .subresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1));

    if (old_image_layout == vk::ImageLayout::eColorAttachmentOptimal) {
        image_memory_barrier.srcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    }
    if (new_image_layout == vk::ImageLayout::eTransferDstOptimal) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask(vk::AccessFlagBits::eMemoryRead);
    }

    if (new_image_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.srcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
    }

    if (new_image_layout == vk::ImageLayout::eColorAttachmentOptimal) {
        image_memory_barrier.dstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
    }

    if (new_image_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        image_memory_barrier.dstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    }

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, { image_memory_barrier });
}

void VKWindow::init_swapchain_extension() {
    assert(size.width() > 0 && size.height() > 0);

    // Construct the surface description:
    vk::Win32SurfaceCreateInfoKHR createInfo;
    createInfo.hinstance(GetModuleHandle(NULL));
    createInfo.hwnd((HWND)winId());
    surface = instance.createWin32SurfaceKHR(createInfo, VK::ALLOC);

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t i = 0;
    for (const auto& queue_properties : physicalDevice.getQueueFamilyProperties()) {
        auto flags = queue_properties.queueFlags();
        if ((vk::QueueFlagBits::eGraphics & flags) && physicalDevice.getSurfaceSupportKHR(i, surface)) {
            graphics_queue_family_index = i;
            break;
        }
        ++i;
    }

    // Generate error if could not find a queue that supports both a graphics
    if (graphics_queue_family_index == UINT32_MAX) {
        qFatal("Could not find a queue that supports both graphics and present");
    }

    // Get the list of VkFormats that are supported:
    std::vector<vk::SurfaceFormatKHR> formats;
    auto res = physicalDevice.getSurfaceFormatsKHR(surface, formats);
    assert(res == vk::Result::eSuccess);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formats.size() == 1 && formats[0].format() == vk::Format::eUndefined) {
        format = vk::Format::eB8G8R8A8Unorm;
    } else {
        assert(formats.size() >= 1);
        format = formats[0].format();
    }
}

void VKWindow::init_presentable_image() {
    Q_ASSERT(swap_chain);
    presentCompleteSemaphore = device.createSemaphore({}, VK::ALLOC);

    // Get the index of the next available swapchain image:
    device.acquireNextImageKHR(swap_chain, UINT64_MAX, presentCompleteSemaphore, {}, current_buffer);
}

void VKWindow::execute_queue_cmdbuf(const vk::CommandBuffer& cmd_bufs) {
    vk::Fence nullFence { nullptr };
    vk::SubmitInfo submit_info;
    submit_info
        .waitSemaphoreCount(1)
        .pWaitSemaphores(&presentCompleteSemaphore)
        .commandBufferCount(1)
        .pCommandBuffers(&cmd_bufs);
    queue.submit({ submit_info }, {});
    queue.waitIdle();
}

void VKWindow::execute_pre_present_barrier() {
    assert(swap_chain);
    vk::ImageMemoryBarrier prePresentBarrier;
    prePresentBarrier
        .srcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .dstAccessMask(vk::AccessFlagBits::eMemoryRead)
        .oldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .newLayout((vk::ImageLayout)(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
        .subresourceRange(vk::ImageSubresourceRange()
            .aspectMask(vk::ImageAspectFlagBits::eColor)
            .layerCount(1)
            .levelCount(1))
        .image(buffers[current_buffer].image);
    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands, 
        vk::PipelineStageFlagBits::eBottomOfPipe, 
        vk::DependencyFlags(), {}, {}, { prePresentBarrier });
}

void VKWindow::execute_present_image() {
    assert(current_buffer != UINT32_MAX);
    assert(swap_chain);
    queue.presentKHR(vk::PresentInfoKHR()
        .swapchainCount(1)
        .pSwapchains(&swap_chain)
        .pImageIndices(&current_buffer));
}

void VKWindow::init_swap_chain() {
    assert(cmd);
    assert(queue);
    vk::Result res;

    vk::SwapchainCreateInfoKHR swap_chain_info;
    swap_chain_info
        .surface(surface)
        .imageFormat(format)
        .clipped(true)
        .imageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .imageArrayLayers(1);
        

    {
        vk::SurfaceCapabilitiesKHR surfCapabilities;
        res = physicalDevice.getSurfaceCapabilitiesKHR(surface, surfCapabilities);
        assert(res == vk::Result::eSuccess);
        // width and height are either both -1, or both not -1.
        if (surfCapabilities.currentExtent().width() == (uint32_t)-1) {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swap_chain_info.imageExtent(size);
        } else {
            // If the surface size is defined, the swap chain size must match
            swap_chain_info.imageExtent(surfCapabilities.currentExtent());
        }

        // Determine the number of VkImage's to use in the swap chain (we desire to
        // own only 1 image at a time, besides the images being displayed and
        // queued for display):
        swap_chain_info.minImageCount(std::min(surfCapabilities.maxImageCount(), surfCapabilities.minImageCount() + 1));

        if (!(vk::SurfaceTransformFlagBitsKHR::eIdentity & surfCapabilities.supportedTransforms())) {
            swap_chain_info.preTransform(surfCapabilities.currentTransform());
        }
    }

    {
        // If mailbox mode is available, use it, as is the lowest-latency non-
        // tearing mode.  If not, try IMMEDIATE which will usually be available,
        // and is fastest (though it tears).  If not, fall back to FIFO which is
        // always available.

        std::set<vk::PresentModeKHR> presentModes;
        {
            std::vector<vk::PresentModeKHR> presentModesVec;
            res = physicalDevice.getSurfacePresentModesKHR(surface, presentModesVec);
            presentModes.insert(presentModesVec.begin(), presentModesVec.end());
        }
        assert(res == vk::Result::eSuccess);
        if (presentModes.count(vk::PresentModeKHR::eMailboxKHR)) {
            swap_chain_info.presentMode(vk::PresentModeKHR::eMailboxKHR);
        } else if (presentModes.count(vk::PresentModeKHR::eImmediateKHR)) {
            swap_chain_info.presentMode(vk::PresentModeKHR::eImmediateKHR);
        } else {
            swap_chain_info.presentMode(vk::PresentModeKHR::eFifoKHR);
        }
    }

    swap_chain = device.createSwapchainKHR(swap_chain_info, VK::ALLOC);
    assert(res == vk::Result::eSuccess);
    std::vector<vk::Image> swapchainImages;
    res = device.getSwapchainImagesKHR(swap_chain, swapchainImages);
    assert(res == vk::Result::eSuccess);
    swapchainImageCount = (uint32_t)swapchainImages.size();
    buffers.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        swap_chain_buffer& sc_buffer = buffers[i];
        sc_buffer.image = swapchainImages[i];
        vk::ImageViewCreateInfo color_image_view;
        color_image_view
            .format(format)
            .image(sc_buffer.image)
            .viewType(vk::ImageViewType::e2D)
            .subresourceRange(vk::ImageSubresourceRange().aspectMask(vk::ImageAspectFlagBits::eColor).layerCount(1).levelCount(1));

        set_image_layout(sc_buffer.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
        sc_buffer.view = device.createImageView(color_image_view, VK::ALLOC);
    }
    current_buffer = 0;
}

void VKWindow::init_uniform_buffer() {

    vk::BufferCreateInfo buf_info;
    buf_info
        .usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .size(sizeof(glm::mat4));
    uniform_data.buf = device.createBuffer(buf_info, VK::ALLOC);

    vk::MemoryRequirements mem_reqs = device.getBufferMemoryRequirements(uniform_data.buf);
    uniform_data.size = mem_reqs.size();
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize(uniform_data.size);
    bool pass;
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, alloc_info);
    assert(pass);
    uniform_data.mem = device.allocateMemory(alloc_info, VK::ALLOC);

    update_uniform_buffer();
    device.bindBufferMemory(uniform_data.buf, uniform_data.mem, 0);

    uniform_data.buffer_info
        .buffer(uniform_data.buf)
        .offset(0)
        .range(sizeof(MVP));
}

void VKWindow::update_uniform_buffer() {
    Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    View = glm::lookAt(
        glm::vec3(5, 3, 10), // Camera is at (5,3,10), in World Space
        glm::vec3(0, 0, 0),  // and looks at the origin
        glm::vec3(0, -1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );
    Model = glm::mat4(1.0f);
    MVP = Projection * View * Model;

    void *pData = (uint8_t*)device.mapMemory(uniform_data.mem, 0, uniform_data.size, vk::MemoryMapFlags());
    memcpy(pData, &MVP, sizeof(MVP));
    device.unmapMemory(uniform_data.mem);
}

void VKWindow::init_descriptor_and_pipeline_layouts(bool use_texture) {
    vk::DescriptorSetLayoutBinding layout_bindings[2];
    layout_bindings[0]
        .binding(0)
        .descriptorType(vk::DescriptorType::eUniformBuffer)
        .descriptorCount(1)
        .stageFlags(vk::ShaderStageFlagBits::eVertex);

    if (use_texture) {
        layout_bindings[1]
            .binding(1)
            .descriptorType(vk::DescriptorType::eCombinedImageSampler)
            .descriptorCount(1)
            .stageFlags(vk::ShaderStageFlagBits::eFragment);
    }

    /* Next take layout bindings and use them to create a descriptor set layout
    */
    vk::DescriptorSetLayoutCreateInfo descriptor_layout;
    descriptor_layout
        .bindingCount(use_texture ? 2 : 1)
        .pBindings(layout_bindings);

    desc_layouts.resize(NUM_DESCRIPTOR_SETS);
    desc_layouts[0] = device.createDescriptorSetLayout(descriptor_layout, VK::ALLOC);

    /* Now use the descriptor layout to create a pipeline layout */
    vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo;
    pPipelineLayoutCreateInfo
        .setLayoutCount(NUM_DESCRIPTOR_SETS)
        .pSetLayouts(desc_layouts.data());

    pipeline_layout = device.createPipelineLayout(pPipelineLayoutCreateInfo, VK::ALLOC);
}

void VKWindow::init_renderpass(bool clear) {
    /* Need attachments for render target and depth buffer */
    vk::AttachmentDescription attachments[2];
    attachments[0]
        .format(format)
        .loadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
        .stencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .stencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .initialLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .finalLayout(vk::ImageLayout::eColorAttachmentOptimal);
    attachments[1]
        .format(depth.format)
        .loadOp(clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare)
        .initialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .finalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depth_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass;
    subpass
        .pipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .colorAttachmentCount(1)
        .pColorAttachments(&color_reference)
        .pDepthStencilAttachment(&depth_reference);
    
    vk::RenderPassCreateInfo rp_info;
    rp_info
        .attachmentCount(2)
        .pAttachments(attachments)
        .subpassCount(1)
        .pSubpasses(&subpass);

    render_pass = device.createRenderPass(rp_info, VK::ALLOC);
}

void VKWindow::init_framebuffers() {
    assert(depth.image);
    assert(render_pass);

    vk::ImageView attachments[2];
    attachments[1] = depth.view;
    vk::FramebufferCreateInfo fb_info;
    fb_info
        .renderPass(render_pass)
        .attachmentCount(2)
        .pAttachments(attachments)
        .width(size.width()).height(size.height()).layers(1);
    framebuffers.resize(swapchainImageCount);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        attachments[0] = buffers[i].view;
        framebuffers[i] = device.createFramebuffer(fb_info, VK::ALLOC);
    }
}

void VKWindow::init_command_pool() {
    /* DEPENDS on init_swapchain_extension() */
    assert(surface);
    vk::CommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info
        .queueFamilyIndex(graphics_queue_family_index)
        .flags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    cmd_pool = device.createCommandPool(cmd_pool_info, VK::ALLOC);
}

void VKWindow::init_command_buffer() {
    /* DEPENDS on init_swapchain_extension() and init_command_pool() */
    assert(surface);
    assert(cmd_pool);
    vk::CommandBufferAllocateInfo cmdBufferAllocateInfo;
    cmdBufferAllocateInfo.commandPool(cmd_pool).level(vk::CommandBufferLevel::ePrimary).commandBufferCount(1);
    cmd_buffers = device.allocateCommandBuffers(cmdBufferAllocateInfo);
    cmd = cmd_buffers[0];
}

void VKWindow::execute_queue_command_buffer() {
    /* Queue the command buffer for execution */
    vk::Fence drawFence = device.createFence(vk::FenceCreateInfo(), VK::ALLOC);
    vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::SubmitInfo submit_info;
    submit_info.pWaitDstStageMask(&pipe_stage_flags).commandBufferCount(1).pCommandBuffers(cmd_buffers.data());
    queue.submit({ submit_info }, drawFence);
    vk::Result res; 
    do {
        res = device.waitForFences({ drawFence }, true, WAIT_TIMEOUT); 
    } while (res == vk::Result::eTimeout);
    Q_ASSERT(res == vk::Result::eSuccess);

    device.destroyFence(drawFence, VK::ALLOC);
}

void VKWindow::init_device_queue() {
    queue = device.getQueue(graphics_queue_family_index, 0);
}

void VKWindow::init_vertex_buffer(const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture) {
    bool pass;

    vk::BufferCreateInfo buf_info;
    buf_info
        .usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .size(dataSize);
    vertex_buffer.buf = device.createBuffer(buf_info, VK::ALLOC);

    vk::MemoryRequirements mem_reqs = device.getBufferMemoryRequirements(vertex_buffer.buf);

    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize(mem_reqs.size());
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        alloc_info);
    assert(pass);

    vertex_buffer.mem = device.allocateMemory(alloc_info, VK::ALLOC);
    vertex_buffer.buffer_info.range(mem_reqs.size());

    {
        void *pData = device.mapMemory(vertex_buffer.mem, 0, mem_reqs.size(), vk::MemoryMapFlags());
        memcpy(pData, vertexData, dataSize);
        device.unmapMemory(vertex_buffer.mem);
    }

    device.bindBufferMemory(vertex_buffer.buf, vertex_buffer.mem, 0);
    vi_binding.binding(0).inputRate(vk::VertexInputRate::eVertex).stride(dataStride);
    vi_attribs[0].format(vk::Format::eR32G32B32A32Sfloat);
    vi_attribs[1].location(1).format(use_texture ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32A32Sfloat).offset(16);
}

void VKWindow::init_descriptor_pool(bool use_texture) {
    assert(uniform_data.buf);
    assert(pipeline_layout);
    assert(!desc_layouts.empty());
    vk::DescriptorPoolSize type_count[2];
    type_count[0].type(vk::DescriptorType::eUniformBuffer).descriptorCount(1);
    if (use_texture) { type_count[1].type(vk::DescriptorType::eCombinedImageSampler).descriptorCount(1); }
    vk::DescriptorPoolCreateInfo descriptor_pool;
    descriptor_pool.maxSets(1).poolSizeCount(use_texture ? 2 : 1).pPoolSizes(type_count);
    desc_pool = device.createDescriptorPool(descriptor_pool, VK::ALLOC);
}

void VKWindow::init_descriptor_set(bool use_texture) {
    /* DEPENDS on init_descriptor_pool() */
    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info
        .descriptorPool(desc_pool)
        .descriptorSetCount(NUM_DESCRIPTOR_SETS)
        .pSetLayouts(desc_layouts.data());
    desc_set = device.allocateDescriptorSets({ alloc_info });

    vk::WriteDescriptorSet writes;
    writes
        .dstSet(desc_set[0])
        .descriptorCount(1)
        .descriptorType(vk::DescriptorType::eUniformBuffer)
        .pBufferInfo(&uniform_data.buffer_info);


    device.updateDescriptorSets({ writes }, {});
}

void VKWindow::init_shaders(const char *vertShaderText, const char *fragShaderText) {
    // If no shaders were submitted, just return
    if (!(vertShaderText || fragShaderText))
        return;

    VK::initGlsl();
    if (vertShaderText) {
        shaderModules[0] = VK::glslToShaderModule(device, vk::ShaderStageFlagBits::eVertex, vertShaderText);
    }
    if (fragShaderText) {
        shaderModules[1] = VK::glslToShaderModule(device, vk::ShaderStageFlagBits::eFragment, fragShaderText);
    }
    VK::finalizeGlsl();
}

void VKWindow::init_pipeline_cache() {
    pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo(), VK::ALLOC);
}

void VKWindow::init_pipeline(VkBool32 include_vi) {
    vk::DynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];

    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.pDynamicStates(dynamicStateEnables);
    dynamicStateEnables[0] = vk::DynamicState::eViewport;
    dynamicStateEnables[1] = vk::DynamicState::eScissor;
    dynamicState.dynamicStateCount(2);

    vk::PipelineViewportStateCreateInfo vp = {};
    vp.viewportCount(NUM_VIEWPORTS);
    vp.scissorCount(NUM_SCISSORS);

    vk::PipelineVertexInputStateCreateInfo vi;
    vi.vertexBindingDescriptionCount(1);
    vi.pVertexBindingDescriptions(&vi_binding);
    vi.vertexAttributeDescriptionCount(2);
    vi.pVertexAttributeDescriptions(vi_attribs);

    vk::PipelineInputAssemblyStateCreateInfo ia;
    ia.topology(vk::PrimitiveTopology::eTriangleList);

    vk::PipelineRasterizationStateCreateInfo rs;
    rs.polygonMode(vk::PolygonMode::eFill);
    rs.cullMode(vk::CullModeFlagBits::eBack);
    rs.frontFace(vk::FrontFace::eClockwise);
    rs.depthClampEnable(true);

    vk::PipelineColorBlendStateCreateInfo cb;
    vk::PipelineColorBlendAttachmentState att_state[1];
    att_state[0].colorWriteMask((vk::ColorComponentFlagBits)0xf);
    cb.attachmentCount(1);
    cb.pAttachments(att_state);
    cb.logicOpEnable(VK_FALSE);
    cb.logicOp(vk::LogicOp::eNoOp);
    cb.blendConstants(std::array<float, 4>{ { 1.0f, 1.0f, 1.0f, 1.0f } });

    vk::PipelineDepthStencilStateCreateInfo ds;
    ds.depthTestEnable(VK_TRUE);
    ds.depthWriteEnable(VK_TRUE);
    ds.depthCompareOp(vk::CompareOp::eLessOrEqual);
    ds.back(vk::StencilOpState().compareOp(vk::CompareOp::eAlways));
    ds.front(ds.back());

    vk::PipelineShaderStageCreateInfo shaderStages[2];
    if (shaderModules[0]) {
        shaderStages[0].stage(vk::ShaderStageFlagBits::eVertex).pName("main").module(shaderModules[0]);
    }
    if (shaderModules[1]) {
        shaderStages[1].stage(vk::ShaderStageFlagBits::eFragment).pName("main").module(shaderModules[1]);
    }

    vk::PipelineMultisampleStateCreateInfo ms;
    vk::GraphicsPipelineCreateInfo pipelineCreate;
    pipelineCreate
        .layout(pipeline_layout)
        .pVertexInputState(include_vi ? &vi : nullptr)
        .pInputAssemblyState(&ia)
        .pRasterizationState(&rs)
        .pColorBlendState(&cb)
        .pMultisampleState(&ms)
        .pDynamicState(&dynamicState)
        .pViewportState(&vp)
        .pDepthStencilState(&ds)
        .pStages(shaderStages)
        .stageCount(2)
        .renderPass(render_pass)
        .subpass(0);
    pipeline = device.createGraphicsPipelines(pipelineCache, { pipelineCreate }, VK::ALLOC)[0];
}

void VKWindow::init_sampler(vk::Sampler &sampler) {
    vk::SamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo
        .addressModeU(vk::SamplerAddressMode::eClampToEdge)
        .addressModeV(vk::SamplerAddressMode::eClampToEdge)
        .addressModeW(vk::SamplerAddressMode::eClampToEdge)
        .borderColor(vk::BorderColor::eFloatOpaqueWhite);
    sampler = device.createSampler(samplerCreateInfo, VK::ALLOC);
}

void VKWindow::init_viewports() {
    viewport.height(size.height()).width(size.width()).minDepth(0).maxDepth(1);
    cmd.setViewport(0, { viewport });
}

void VKWindow::init_scissors() {
    scissor.extent(size);
    cmd.setScissor(0, { scissor });
}

void VKWindow::init_fence(vk::Fence &fence) {
    fence = device.createFence(vk::FenceCreateInfo(), VK::ALLOC);
}

void VKWindow::init_submit_info(vk::SubmitInfo &submit_info,
    vk::PipelineStageFlags &pipe_stage_flags) {
    submit_info.waitSemaphoreCount(1)
        .pWaitSemaphores(&presentCompleteSemaphore)
        .pWaitDstStageMask(&pipe_stage_flags)
        .commandBufferCount(1)
        .pCommandBuffers(&cmd);
}

void VKWindow::init_present_info(vk::PresentInfoKHR &present) {
    present.swapchainCount(1).pSwapchains(&swap_chain).pImageIndices(&current_buffer);
}

void VKWindow::destroy_pipeline() {
    device.destroyPipeline(pipeline, VK::ALLOC);
}

void VKWindow::destroy_pipeline_cache() {
    device.destroyPipelineCache(pipelineCache, VK::ALLOC);
}

void VKWindow::destroy_uniform_buffer() {
    device.destroyBuffer(uniform_data.buf, VK::ALLOC);
    device.freeMemory(uniform_data.mem, VK::ALLOC);
}

void VKWindow::destroy_descriptor_and_pipeline_layouts() {
    for (const auto& desc_layout : desc_layouts) {
        device.destroyDescriptorSetLayout(desc_layout, VK::ALLOC);
    }
    desc_layouts.clear();
    device.destroyPipelineLayout(pipeline_layout, VK::ALLOC);
}

void VKWindow::destroy_descriptor_pool() {
    device.destroyDescriptorPool(desc_pool, VK::ALLOC);
}

void VKWindow::destroy_shaders() {
    for (auto& shaderModule : shaderModules) {
        device.destroyShaderModule(shaderModule, VK::ALLOC);
        shaderModule = vk::ShaderModule();
    }
}

void VKWindow::destroy_command_buffer() {
    device.freeCommandBuffers(cmd_pool, { cmd });
}

void VKWindow::destroy_command_pool() {
    device.destroyCommandPool(cmd_pool, VK::ALLOC);
}

void VKWindow::destroy_depth_buffer() {
    device.destroyImageView(depth.view, VK::ALLOC);
    device.destroyImage(depth.image, VK::ALLOC);
    device.freeMemory(depth.mem, VK::ALLOC);
}

void VKWindow::destroy_vertex_buffer() {
    device.destroyBuffer(vertex_buffer.buf, VK::ALLOC);
    device.freeMemory(vertex_buffer.mem, VK::ALLOC);
}

void VKWindow::destroy_swap_chain() {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyImageView(device, buffers[i].view, NULL);
    }
    vkDestroySwapchainKHR(device, swap_chain, NULL);
}

void VKWindow::destroy_framebuffers() {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
    }
    framebuffers.clear();
}

void VKWindow::destroy_renderpass() {
    vkDestroyRenderPass(device, render_pass, NULL);
}

void VKWindow::destroy_device() {
    vkDestroyDevice(device, NULL);
}

void VKWindow::destroy_instance() {
    vkDestroyInstance(instance, NULL);
}

static const char *vertShaderText =
R"SHADER(#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;
layout (location = 0) out vec4 outColor;
out gl_PerVertex { 
    vec4 gl_Position;
};
void main() {
   outColor = inColor;
   gl_Position = myBufferVals.mvp * pos;

   // GL->VK conventions
   gl_Position.y = -gl_Position.y;
   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
)SHADER";

static const char *fragShaderText =
R"SHADER(#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;
void main() {
   outColor = color;
}
)SHADER";


VKWindow::VKWindow(QObject* parent) {
    installEventFilter(new VkCloseEventFilter(this));
    const bool depthPresent = true;

    init_instance("ShadertoyVR");
    init_physical_device();
    init_window_size(500, 500);
    init_swapchain_extension();
    init_device();

    init_command_pool();
    init_command_buffer();
    cmd.begin(vk::CommandBufferBeginInfo());
    init_device_queue();
    init_swap_chain();
    init_depth_buffer();
    init_uniform_buffer();
    init_descriptor_and_pipeline_layouts(false);
    init_renderpass();
    init_shaders(vertShaderText, fragShaderText);
    init_framebuffers();
    init_vertex_buffer(
        g_vb_solid_face_colors_Data,
        sizeof(g_vb_solid_face_colors_Data),
        sizeof(g_vb_solid_face_colors_Data[0]), false);
    init_descriptor_pool(false);
    init_descriptor_set(false);
    init_pipeline_cache();
    init_pipeline();
    draw();
}

void VKWindow::draw() {
    /* VULKAN_KEY_START */
    vk::ClearValue clear_values[2];
    static std::array<float, 4Ui64> clear_color { { 0.2f, 0.2f, 0.2f, 0.2f } };
    clear_values[0].color(clear_color);
    clear_values[1].depthStencil({ 1.0f, 0 });

    vk::Semaphore presentCompleteSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo(), VK::ALLOC);

    auto res = device.acquireNextImageKHR(swap_chain, UINT64_MAX, presentCompleteSemaphore, vk::Fence(), current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == vk::Result::eSuccess);

    vk::RenderPassBeginInfo rp_begin;
    rp_begin
        .renderPass(render_pass)
        .framebuffer(framebuffers[current_buffer])
        .renderArea(vk::Rect2D(vk::Offset2D(), size))
        .clearValueCount(2)
        .pClearValues(clear_values);

    cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, desc_set, {});

    cmd.bindVertexBuffers(0, { vertex_buffer.buf }, { 0 });

    init_viewports();
    init_scissors();

    cmd.draw(12 * 3, 1, 0, 0);
    cmd.endRenderPass();

    vk::ImageMemoryBarrier prePresentBarrier;
    prePresentBarrier
        .srcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .dstAccessMask(vk::AccessFlagBits::eMemoryRead)
        .oldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .newLayout(vk::ImageLayout::ePresentSrcKHR)
        .subresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
        .image(buffers[current_buffer].image);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, { prePresentBarrier });
    cmd.end();

    vk::Fence drawFence = device.createFence(vk::FenceCreateInfo(), VK::ALLOC);
    vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::SubmitInfo submit_info;
    submit_info
        .waitSemaphoreCount(1)
        .pWaitSemaphores(&presentCompleteSemaphore)
        .pWaitDstStageMask(&pipe_stage_flags)
        .commandBufferCount(1)
        .pCommandBuffers(cmd_buffers.data());
    /* Queue the command buffer for execution */
    queue.submit({ submit_info }, drawFence);

    /* Now present the image in the window */

    vk::PresentInfoKHR present;
    present
        .swapchainCount(1)
        .pSwapchains(&swap_chain)
        .pImageIndices(&current_buffer);

    /* Make sure command buffer is finished before presenting */
    res = vk::Result::eTimeout;
    while (vk::Result::eTimeout == res) {
        res = device.waitForFences({ drawFence }, VK_TRUE, FENCE_TIMEOUT);
    }
    assert(vk::Result::eSuccess == res);
    res = queue.presentKHR(present);
    assert(vk::Result::eSuccess == res);

    /* VULKAN_KEY_END */
    device.destroySemaphore(presentCompleteSemaphore, VK::ALLOC);
    device.destroyFence(drawFence, VK::ALLOC);
}

VKWindow::~VKWindow() {
    destroy_pipeline();
    destroy_pipeline_cache();
    destroy_descriptor_pool();
    destroy_vertex_buffer();
    destroy_framebuffers();
    destroy_shaders();
    destroy_renderpass();
    destroy_descriptor_and_pipeline_layouts();
    destroy_uniform_buffer();
    destroy_depth_buffer();
    destroy_swap_chain();
    destroy_command_buffer();
    destroy_command_pool();
    destroy_window();
    destroy_device();
    destroy_instance();
}

void VKWindow::emitClosing() {
    emit aboutToClose();
}


#include "VKWindow.moc"
