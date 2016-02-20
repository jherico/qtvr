//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VKWindow.h"

#include <QtCore/QCoreApplication>
#include <QtGui/qevent.h>
#include <QtCore/QDebug>

#include <SPIRV/GlslangToSpv.h>

#include "CubeData.h"

VkBool32 debugReport(
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

static vk::AllocationCallbacks DEFAULT_ALLOC(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

vk::Result VKWindow::init_global_extension_properties(layer_properties &layer_props) {
    vk::Result res = vk::enumerateInstanceExtensionProperties(std::string(layer_props.properties.layerName), layer_props.extensions);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    return res;
}

vk::Result VKWindow::init_global_layer_properties() {
    vk::Result res;
    std::vector<vk::LayerProperties> layers_properties;
    res = vk::enumerateInstanceLayerProperties(layers_properties);
    instance_layer_properties.resize(layers_properties.size());
    for (uint32_t i = 0; i < layers_properties.size(); ++i) {
        layer_properties& layer_props = instance_layer_properties[i];
        layer_props.properties = layers_properties[i];
        res = init_global_extension_properties(layer_props);
        Q_ASSERT(res == vk::Result::eVkSuccess);
    }
    return res;
}

vk::Result VKWindow::init_device_extension_properties(layer_properties &layer_props) {
    char *layer_name = layer_props.properties.layerName;
    vk::Result res = vk::enumerateDeviceExtensionProperties(gpus[0], layer_name, layer_props.extensions);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    return res;
}

vk::Result VKWindow::init_device_layer_properties() {
    //this->device_layer_properties
    std::vector<vk::LayerProperties> layers_properties;
    vk::Result res = vk::enumerateDeviceLayerProperties(gpus[0], layers_properties);
    for (uint32_t i = 0; i < layers_properties.size(); i++) {
        layer_properties layer_props;
        layer_props.properties = layers_properties[i];
        res = init_device_extension_properties(layer_props);
        Q_ASSERT(res == vk::Result::eVkSuccess);
        device_layer_properties.push_back(layer_props);
    }
    return res;
}

VkBool32 VKWindow::demo_check_layers(const std::vector<layer_properties> &layer_props, const std::vector<const char *> &layer_names) {
    uint32_t check_count = (uint32_t)layer_names.size();
    uint32_t layer_count = (uint32_t)layer_props.size();
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(layer_names[i], layer_props[j].properties.layerName)) {
                found = 1;
            }
        }
        if (!found) {
            qDebug() << "Cannot find layer: " << layer_names[i] ;
            return 0;
        }
    }
    return 1;
}

void VKWindow::init_instance_extension_names() {
    instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
}

vk::Result VKWindow::init_instance(char const *const app_short_name) {
    vk::ApplicationInfo app_info;
    app_info
        .pApplicationName(app_short_name)
        .pEngineName(app_short_name)
        .applicationVersion(1)
        .engineVersion(1)
        .apiVersion(VK_API_VERSION);

    instance_layer_names.push_back("VK_LAYER_LUNARG_threading");
    instance_layer_names.push_back("VK_LAYER_LUNARG_draw_state");
    instance_layer_names.push_back("VK_LAYER_LUNARG_image");
    instance_layer_names.push_back("VK_LAYER_LUNARG_mem_tracker");
    instance_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
    instance_layer_names.push_back("VK_LAYER_LUNARG_param_checker");
    instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    vk::InstanceCreateInfo inst_info;
    inst_info
        .pApplicationInfo(&app_info)
        .enabledLayerCount((uint32_t)instance_layer_names.size())
        .enabledExtensionCount((uint32_t)instance_extension_names.size())
        .ppEnabledExtensionNames(instance_extension_names.data());

    if (!instance_layer_names.empty()) {
        inst_info.ppEnabledLayerNames(instance_layer_names.data());
    }

    vk::Result res = vk::createInstance(inst_info, DEFAULT_ALLOC, inst);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    //// Triggers validation error!
    //VkDebugReportCallbackEXT debug_report_callback;
    //dbgCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT");
    //if (!dbgCreateDebugReportCallback) {
    //    qFatal("GetInstanceProcAddr: Unable to find vkCreateDebugReportCallbackEXT function.");
    //}

    //dbgDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT");
    //if (!dbgDestroyDebugReportCallback) {
    //    qFatal("GetInstanceProcAddr: Unable to find vkDestroyDebugReportCallbackEXT function.");
    //}

    //vk::DebugReportFlagsEXT debug_flags;
    //debug_flags |= vk::DebugReportFlagBitsEXT::eError;
    //debug_flags |= vk::DebugReportFlagBitsEXT::eWarning;
    //vk::DebugReportCallbackCreateInfoEXT create_info(debug_flags, debugReport, this);
    //VkResult res2 = dbgCreateDebugReportCallback(inst, &(VkDebugReportCallbackCreateInfoEXT&)create_info, NULL,
    //    &debug_report_callback);
    //switch (res2) {
    //case VK_SUCCESS:
    //    break;
    //case VK_ERROR_OUT_OF_HOST_MEMORY:
    //    qFatal("dbgCreateDebugReportCallback: out of host memory");
    //    break;
    //default:
    //    qFatal("dbgCreateDebugReportCallback: unknown failure");
    //    break;
    //}
    return res;
}

void VKWindow::init_device_extension_names() {
    device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

vk::Result VKWindow::init_device() {
    vk::Result res;

    float queue_priorities[1] = { 0.0 };
    vk::DeviceQueueCreateInfo queue_info;
    queue_info
        .queueCount(1)
        .queueFamilyIndex(graphics_queue_family_index)
        .pQueuePriorities(queue_priorities);

    device_layer_names.push_back("VK_LAYER_LUNARG_threading");
    device_layer_names.push_back("VK_LAYER_LUNARG_draw_state");
    device_layer_names.push_back("VK_LAYER_LUNARG_image");
    device_layer_names.push_back("VK_LAYER_LUNARG_mem_tracker");
    device_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
    device_layer_names.push_back("VK_LAYER_LUNARG_param_checker");


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
    res = vk::createDevice(gpus[0], device_info, DEFAULT_ALLOC, device);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    return res;
}

vk::Result VKWindow::init_enumerate_device(uint32_t gpu_count) {
    uint32_t const req_count = gpu_count;

    vk::Result res = vk::enumeratePhysicalDevices(inst, gpus);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    queue_count = (uint32_t)gpus.size();

    queue_props = vk::getPhysicalDeviceQueueFamilyProperties(gpus[0]);
    assert(!queue_props.empty());

    /* This is as good a place as any to do this */
    vk::getPhysicalDeviceMemoryProperties(gpus[0], memory_properties);
    vk::getPhysicalDeviceProperties(gpus[0], gpu_props);
    return res;
}

void VKWindow::destroy_window() {
    vk::destroySurfaceKHR(inst, surface, NULL);
}

void VKWindow::init_window_size(int32_t default_width, int32_t default_height) {
    width = default_width;
    height = default_height;
    setGeometry(100, 100, default_width, default_height);
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
    vk::Result res;
    bool pass;

    vk::ImageCreateInfo image_info;
    const vk::Format depth_format = depth.format;

    vk::FormatProperties props;
    vk::getPhysicalDeviceFormatProperties(gpus[0], depth_format, props);

    if (props.linearTilingFeatures() & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        image_info.tiling(vk::ImageTiling::eLinear);
    } else if (props.optimalTilingFeatures() & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        image_info.tiling(vk::ImageTiling::eOptimal);
    } else {
        qFatal("depth_format unsupported.");
    }

    image_info
        .imageType(vk::ImageType::e2D)
        .format(depth_format)
        .extent(vk::Extent3D(width, height, 1))
        .mipLevels(1)
        .arrayLayers(1)
        .usage(vk::ImageUsageFlagBits::eDepthStencilAttachment);

    /* Create image */
    res = vk::createImage(device, image_info, DEFAULT_ALLOC, depth.image);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::MemoryRequirements mem_reqs;
    vk::getImageMemoryRequirements(device, depth.image, mem_reqs);

    vk::MemoryAllocateInfo mem_alloc;
    mem_alloc.allocationSize(mem_reqs.size());
    /* Use the memory properties to determine the type of memory required */
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(), 0, mem_alloc);
    assert(pass);

    /* Allocate memory */
    res = vk::allocateMemory(device, mem_alloc, DEFAULT_ALLOC, depth.mem);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    /* Bind memory */
    res = vk::bindImageMemory(device, depth.image, depth.mem, 0);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::ImageViewCreateInfo view_info;
    view_info
        .format(depth_format)
        .subresourceRange(vk::ImageSubresourceRange()
        .aspectMask(vk::ImageAspectFlagBits::eDepth)
        .levelCount(1)
        .layerCount(1))
        .viewType(vk::ImageViewType::e2D);

    /* Set the image layout to depth stencil optimal */
    set_image_layout(depth.image,
        view_info.subresourceRange().aspectMask(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* Create image view */
    view_info.image(depth.image);
    res = vk::createImageView(device, view_info, DEFAULT_ALLOC, depth.view);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout) {
    /* DEPENDS on cmd and queue initialized */

    assert(cmd != VK_NULL_HANDLE);
    assert(queue != VK_NULL_HANDLE);

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = aspectMask;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.layerCount = 1;

    if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.srcAccessMask =
            VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(cmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL,
        1, &image_memory_barrier);
}

void VKWindow::init_swapchain_extension() {
    Q_ASSERT(width > 0 && height > 0);
    vk::Result res;

    // Construct the surface description:
    vk::Win32SurfaceCreateInfoKHR createInfo;
    createInfo.hinstance(GetModuleHandle(NULL));
    createInfo.hwnd((HWND)winId());
    res = vk::createWin32SurfaceKHR(inst, createInfo, DEFAULT_ALLOC, surface);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    for (uint32_t i = 0; i < queue_count; i++) {
        if ((queue_props[i].queueFlags() & VK_QUEUE_GRAPHICS_BIT) != 0) {
            vk::Bool32 presentSupported;
            vk::getPhysicalDeviceSurfaceSupportKHR(gpus[0], i, surface, presentSupported);
            if (presentSupported) {
                graphics_queue_family_index = i;
                break;
            }
        }
    }

    // Generate error if could not find a queue that supports both a graphics
    if (graphics_queue_family_index == UINT32_MAX) {
        qFatal("Could not find a queue that supports both graphics and present");
    }

    // Get the list of VkFormats that are supported:
    std::vector<vk::SurfaceFormatKHR> formats;
    res = vk::getPhysicalDeviceSurfaceFormatsKHR(gpus[0], surface, formats);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formats.size() == 1 && formats[0].format() == vk::Format::eUndefined) {
        format = vk::Format::eB8G8R8A8Unorm;
    } else {
        Q_ASSERT(formats.size() >= 1);
        format = formats[0].format();
    }
}

void VKWindow::init_presentable_image() {
    Q_ASSERT(swap_chain);
    vk::Result res;
    res = vk::createSemaphore(device, vk::SemaphoreCreateInfo(), DEFAULT_ALLOC, presentCompleteSemaphore);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    // Get the index of the next available swapchain image:
    res = vk::acquireNextImageKHR(device, swap_chain, UINT64_MAX, presentCompleteSemaphore, nullptr, current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR return codes
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::execute_queue_cmdbuf(const vk::CommandBuffer *cmd_bufs) {
    vk::Result res;
    vk::Fence nullFence { nullptr };

    vk::SubmitInfo submit_info[1];
    submit_info[0]
        .waitSemaphoreCount(1)
        .pWaitSemaphores(&presentCompleteSemaphore)
        .commandBufferCount(1)
        .pCommandBuffers(cmd_bufs);

    /* Queue the command buffer for execution */
    res = vk::queueSubmit(queue, 1, submit_info, nullptr);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    res = vk::queueWaitIdle(queue);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::execute_pre_present_barrier() {
    Q_ASSERT(swap_chain);
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
    vk::cmdPipelineBarrier(cmd,
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eBottomOfPipe, 
        vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &prePresentBarrier);
}

void VKWindow::execute_present_image() {
    Q_ASSERT(current_buffer != UINT32_MAX);
    Q_ASSERT(swap_chain);
    vk::Result res = vk::queuePresentKHR(queue, 
        vk::PresentInfoKHR()
            .swapchainCount(1)
            .pSwapchains(&swap_chain)
            .pImageIndices(&current_buffer));
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI return codes
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_swap_chain() {
    Q_ASSERT(cmd);
    Q_ASSERT(queue);
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
        res = vk::getPhysicalDeviceSurfaceCapabilitiesKHR(gpus[0], surface, surfCapabilities);
        Q_ASSERT(res == vk::Result::eVkSuccess);
        // width and height are either both -1, or both not -1.
        if (surfCapabilities.currentExtent().width() == (uint32_t)-1) {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swap_chain_info.imageExtent(vk::Extent2D(width, height));
        } else {
            // If the surface size is defined, the swap chain size must match
            swap_chain_info.imageExtent(surfCapabilities.currentExtent());
        }


        // Determine the number of VkImage's to use in the swap chain (we desire to
        // own only 1 image at a time, besides the images being displayed and
        // queued for display):
        swap_chain_info.minImageCount(std::min(surfCapabilities.maxImageCount(), surfCapabilities.minImageCount() + 1));

        
        if (!(surfCapabilities.supportedTransforms() & vk::SurfaceTransformFlagBitsKHR::eIdentity)) {
            swap_chain_info.preTransform(surfCapabilities.currentTransform());
        }
    }

    {
        // If mailbox mode is available, use it, as is the lowest-latency non-
        // tearing mode.  If not, try IMMEDIATE which will usually be available,
        // and is fastest (though it tears).  If not, fall back to FIFO which is
        // always available.
        std::vector<vk::PresentModeKHR> presentModes;
        res = vk::getPhysicalDeviceSurfacePresentModesKHR(gpus[0], surface, presentModes);
        Q_ASSERT(res == vk::Result::eVkSuccess);
        swap_chain_info.presentMode(vk::PresentModeKHR::eVkPresentModeFifoKhr);
        for (const auto& presentMode : presentModes) {
            if (presentMode == vk::PresentModeKHR::eVkPresentModeMailboxKhr) {
                swap_chain_info.presentMode(presentMode);
                break;
            } else if (presentMode == vk::PresentModeKHR::eVkPresentModeImmediateKhr) {
                swap_chain_info.presentMode(presentMode);
            }
        }
    }

    res = vk::createSwapchainKHR(device, swap_chain_info, DEFAULT_ALLOC, swap_chain);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    std::vector<vk::Image> swapchainImages;
    res = vk::getSwapchainImagesKHR(device, swap_chain, swapchainImages);
    Q_ASSERT(res == vk::Result::eVkSuccess);
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

        set_image_layout(sc_buffer.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        res = vk::createImageView(device, color_image_view, DEFAULT_ALLOC, sc_buffer.view);
        Q_ASSERT(res == vk::Result::eVkSuccess);
    }
    current_buffer = 0;
}

void VKWindow::init_uniform_buffer() {
    vk::Result res;
    bool pass;

    vk::BufferCreateInfo buf_info;
    buf_info
        .usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .size(sizeof(glm::mat4));
    res = vk::createBuffer(device, buf_info, DEFAULT_ALLOC, uniform_data.buf);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::MemoryRequirements mem_reqs;
    vk::getBufferMemoryRequirements(device, uniform_data.buf, mem_reqs);
    uniform_data.size = mem_reqs.size();
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize(uniform_data.size);
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, alloc_info);
    assert(pass);
    res = vk::allocateMemory(device, alloc_info, DEFAULT_ALLOC, uniform_data.mem);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    update_uniform_buffer();

    res = vk::bindBufferMemory(device, uniform_data.buf,
        uniform_data.mem, 0);
    Q_ASSERT(res == vk::Result::eVkSuccess);

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

    uint8_t *pData;
    vk::Result res;
    res = vk::mapMemory(device, uniform_data.mem, 0, uniform_data.size, 0, (void **)&pData);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    memcpy(pData, &MVP, sizeof(MVP));
    vk::unmapMemory(device, uniform_data.mem);
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

    vk::Result res;
    desc_layout.resize(NUM_DESCRIPTOR_SETS);
    res = vk::createDescriptorSetLayout(device, descriptor_layout, DEFAULT_ALLOC, desc_layout[0]);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    /* Now use the descriptor layout to create a pipeline layout */
    vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo;
    pPipelineLayoutCreateInfo
        .setLayoutCount(NUM_DESCRIPTOR_SETS)
        .pSetLayouts(desc_layout.data());

    res = vk::createPipelineLayout(device, pPipelineLayoutCreateInfo, DEFAULT_ALLOC, pipeline_layout);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_renderpass(bool include_depth, bool clear) {
    /* Need attachments for render target and depth buffer */
    vk::AttachmentDescription attachments[2];
    attachments[0]
        .format(format)
        .samples((vk::SampleCountFlagBits)NUM_SAMPLES)
        .loadOp((vk::AttachmentLoadOp)(clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE))
        .storeOp((vk::AttachmentStoreOp)VK_ATTACHMENT_STORE_OP_STORE)
        .stencilLoadOp((vk::AttachmentLoadOp)VK_ATTACHMENT_LOAD_OP_DONT_CARE)
        .stencilStoreOp((vk::AttachmentStoreOp)VK_ATTACHMENT_STORE_OP_DONT_CARE)
        .initialLayout((vk::ImageLayout)VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .finalLayout((vk::ImageLayout)VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    if (include_depth) {
        attachments[1]
            .format(depth.format)
            .samples((vk::SampleCountFlagBits)NUM_SAMPLES)
            .loadOp((vk::AttachmentLoadOp)(clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE))
            .storeOp((vk::AttachmentStoreOp)VK_ATTACHMENT_STORE_OP_STORE)
            .stencilLoadOp((vk::AttachmentLoadOp)VK_ATTACHMENT_LOAD_OP_LOAD)
            .stencilStoreOp((vk::AttachmentStoreOp)VK_ATTACHMENT_STORE_OP_STORE)
            .initialLayout((vk::ImageLayout)VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .finalLayout((vk::ImageLayout)VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    vk::AttachmentReference color_reference;
    color_reference.attachment(0).layout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depth_reference;
    depth_reference.attachment(1).layout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    
    vk::SubpassDescription subpass = {};
    subpass
        .pipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .colorAttachmentCount(1)
        .pColorAttachments(&color_reference);
    if (include_depth) {
        subpass.pDepthStencilAttachment(&depth_reference);
    } 
    
    vk::RenderPassCreateInfo rp_info;
    rp_info
        .attachmentCount(include_depth ? 2 : 1)
        .pAttachments(attachments)
        .subpassCount(1)
        .pSubpasses(&subpass);

    auto res = vk::createRenderPass(device, rp_info, DEFAULT_ALLOC, render_pass);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_framebuffers(bool include_depth) {
    Q_ASSERT(depth.image);
    Q_ASSERT(render_pass);

    vk::Result res;
    vk::ImageView attachments[2];
    attachments[1] = depth.view;

    vk::FramebufferCreateInfo fb_info;
    fb_info
        .renderPass(render_pass)
        .attachmentCount(include_depth ? 2 : 1)
        .pAttachments(attachments)
        .width(width).height(height).layers(1);
    framebuffers.resize(swapchainImageCount);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        attachments[0] = buffers[i].view;
        res = vk::createFramebuffer(device, fb_info, DEFAULT_ALLOC, framebuffers[i]);
        Q_ASSERT(res == vk::Result::eVkSuccess);
    }
}

void VKWindow::init_command_pool() {
    /* DEPENDS on init_swapchain_extension() */

    vk::CommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.queueFamilyIndex(graphics_queue_family_index).flags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    vk::Result res = vk::createCommandPool(device, cmd_pool_info, DEFAULT_ALLOC, cmd_pool);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_command_buffer() {
    /* DEPENDS on init_swapchain_extension() and init_command_pool() */

    vk::CommandBufferAllocateInfo cmd;
    cmd.commandPool(cmd_pool).level(vk::CommandBufferLevel::ePrimary).commandBufferCount(1);
    auto res = vk::allocateCommandBuffers(device, &cmd, &this->cmd);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::execute_begin_command_buffer() {
    auto res = vk::beginCommandBuffer(this->cmd, vk::CommandBufferBeginInfo());
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::execute_end_command_buffer() {
    auto res = vk::endCommandBuffer(cmd);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::execute_queue_command_buffer() {
    /* Queue the command buffer for execution */
    vk::Fence drawFence { nullptr };
    vk::createFence(device, vk::FenceCreateInfo(), DEFAULT_ALLOC, drawFence);

    const vk::CommandBuffer cmd_bufs[] = { cmd };
    vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::SubmitInfo submit_info[1];
    submit_info[0].pWaitDstStageMask(&pipe_stage_flags).commandBufferCount(1).pCommandBuffers(cmd_bufs);
    auto res = vk::queueSubmit(queue, 1, submit_info, drawFence);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    do {
        res = vk::waitForFences(device, 1, &drawFence, true, FENCE_TIMEOUT);
    } while (res == vk::Result::eVkTimeout);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::destroyFence(device, drawFence, NULL);
}

void VKWindow::init_device_queue() {
    vk::getDeviceQueue(device, graphics_queue_family_index, 0, queue);
}

void VKWindow::init_vertex_buffer(const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture) {
    bool pass;

    vk::BufferCreateInfo buf_info;

    buf_info
        .usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .size(dataSize);
    auto res = vk::createBuffer(device, buf_info, DEFAULT_ALLOC, vertex_buffer.buf);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::MemoryRequirements mem_reqs;
    vk::getBufferMemoryRequirements(device, vertex_buffer.buf, mem_reqs);

    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize(mem_reqs.size());
    pass = memory_type_from_properties(mem_reqs.memoryTypeBits(),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        alloc_info);
    assert(pass);

    res = vk::allocateMemory(device, alloc_info, DEFAULT_ALLOC, vertex_buffer.mem);
    Q_ASSERT(res == vk::Result::eVkSuccess);
    vertex_buffer.buffer_info.range = mem_reqs.size();
    vertex_buffer.buffer_info.offset = 0;

    {
        uint8_t *pData;
        res = vk::mapMemory(device, vertex_buffer.mem, 0, mem_reqs.size(), 0, (void **)&pData);
        Q_ASSERT(res == vk::Result::eVkSuccess);
        memcpy(pData, vertexData, dataSize);
        vk::unmapMemory(device, vertex_buffer.mem);
    }

    res = vk::bindBufferMemory(device, vertex_buffer.buf, vertex_buffer.mem, 0);
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vi_binding.binding(0).inputRate(vk::VertexInputRate::eVertex).stride(dataStride);

    vi_attribs[0].format(vk::Format::eR32G32B32A32Sfloat);
    vi_attribs[1].location(1).format(use_texture ? vk::Format::eR32G32Sfloat : vk::Format::eR32G32B32A32Sfloat).offset(16);
}

void VKWindow::init_descriptor_pool(bool use_texture) {
    Q_ASSERT(uniform_data.buf);
    Q_ASSERT(pipeline_layout);
    Q_ASSERT(!desc_layout.empty());
    vk::DescriptorPoolSize type_count[2];
    type_count[0].type(vk::DescriptorType::eUniformBuffer).descriptorCount(1);
    if (use_texture) { type_count[1].type(vk::DescriptorType::eCombinedImageSampler).descriptorCount(1); }
    vk::DescriptorPoolCreateInfo descriptor_pool;
    descriptor_pool.maxSets(1).poolSizeCount(use_texture ? 2 : 1).pPoolSizes(type_count);
    auto res = vk::createDescriptorPool(device, descriptor_pool, DEFAULT_ALLOC, desc_pool);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_descriptor_set(bool use_texture) {
    /* DEPENDS on init_descriptor_pool() */

    vk::Result res;

    vk::DescriptorSetAllocateInfo alloc_info[1];
    alloc_info[0]
        .descriptorPool(desc_pool)
        .descriptorSetCount(NUM_DESCRIPTOR_SETS)
        .pSetLayouts(desc_layout.data());
    desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vk::allocateDescriptorSets(device, alloc_info, desc_set.data());
    Q_ASSERT(res == vk::Result::eVkSuccess);

    vk::WriteDescriptorSet writes[2];
    writes[0]
        .dstSet(desc_set[0])
        .descriptorCount(1)
        .descriptorType(vk::DescriptorType::eUniformBuffer)
        .pBufferInfo(&uniform_data.buffer_info);


    vk::updateDescriptorSets(device, use_texture ? 2 : 1, writes, 0, NULL);
}

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

EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type) {
    switch (shader_type) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return EShLangVertex;

    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return EShLangTessControl;

    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return EShLangTessEvaluation;

    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return EShLangGeometry;

    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return EShLangFragment;

    case VK_SHADER_STAGE_COMPUTE_BIT:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}
//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv) {
    glslang::TProgram &program = *new glslang::TProgram;
    const char *shaderStrings[1];
    TBuiltInResource Resources;
    init_resources(Resources);

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    EShLanguage stage = FindLanguage(shader_type);
    glslang::TShader *shader = new glslang::TShader(stage);

    shaderStrings[0] = pshader;
    shader->setStrings(shaderStrings, 1);

    if (!shader->parse(&Resources, 100, false, messages)) {
        puts(shader->getInfoLog());
        puts(shader->getInfoDebugLog());
        return false; // something didn't work
    }

    program.addShader(shader);

    //
    // Program-level processing...
    //

    if (!program.link(messages)) {
        puts(shader->getInfoLog());
        puts(shader->getInfoDebugLog());
        return false;
    }

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    return true;
}

void VKWindow::init_shaders(const char *vertShaderText, const char *fragShaderText) {
    // If no shaders were submitted, just return
    if (!(vertShaderText || fragShaderText))
        return;

    vk::Result res;
    bool retVal;
    glslang::InitializeProcess();
    vk::ShaderModuleCreateInfo moduleCreateInfo;

    if (vertShaderText) {
        std::vector<unsigned int> vtx_spv;
        shaderStages[0].stage(vk::ShaderStageFlagBits::eVertex).pName("main");
        retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderText, vtx_spv);
        assert(retVal);

        moduleCreateInfo.codeSize(vtx_spv.size() * sizeof(unsigned int)).pCode(vtx_spv.data());
        vk::ShaderModule module;
        res = vk::createShaderModule(device, moduleCreateInfo, DEFAULT_ALLOC, module);
        shaderStages[0].module(module);
        Q_ASSERT(res == vk::Result::eVkSuccess);
    }

    if (fragShaderText) {
        std::vector<unsigned int> frag_spv;
        shaderStages[1].stage(vk::ShaderStageFlagBits::eFragment).pName("main");

        retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText, frag_spv);
        assert(retVal);

        moduleCreateInfo.codeSize(frag_spv.size() * sizeof(unsigned int)).pCode(frag_spv.data());
        vk::ShaderModule module;
        res = vk::createShaderModule(device, moduleCreateInfo, DEFAULT_ALLOC, module);
        shaderStages[1].module(module);
        Q_ASSERT(res == vk::Result::eVkSuccess);
    }

    glslang::FinalizeProcess();
}

void VKWindow::init_pipeline_cache() {
    auto res = vk::createPipelineCache(device, vk::PipelineCacheCreateInfo(), DEFAULT_ALLOC, pipelineCache);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_pipeline(VkBool32 include_depth, VkBool32 include_vi) {
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
    rs.depthClampEnable(include_depth);

    vk::PipelineColorBlendStateCreateInfo cb;
    vk::PipelineColorBlendAttachmentState att_state[1];
    att_state[0].colorWriteMask((vk::ColorComponentFlagBits)0xf);
    cb.attachmentCount(1);
    cb.pAttachments(att_state);
    cb.logicOpEnable( VK_FALSE);
    cb.logicOp(vk::LogicOp::eNoOp);
    cb.blendConstants(std::array<float, 4>{ { 1.0f, 1.0f, 1.0f, 1.0f } });

    vk::PipelineDepthStencilStateCreateInfo ds;
    ds.depthTestEnable(include_depth);
    ds.depthWriteEnable(include_depth);
    ds.depthCompareOp(vk::CompareOp::eLessOrEqual);
    ds.back(vk::StencilOpState().compareOp(vk::CompareOp::eAlways));
    ds.front(ds.back());

    vk::PipelineMultisampleStateCreateInfo ms;
    vk::GraphicsPipelineCreateInfo pipeline;
    pipeline.layout(pipeline_layout);
    pipeline.pVertexInputState(include_vi ? &vi : nullptr);
    pipeline.pInputAssemblyState(&ia);
    pipeline.pRasterizationState(&rs);
    pipeline.pColorBlendState(&cb);
    pipeline.pMultisampleState(&ms);
    pipeline.pDynamicState(&dynamicState);
    pipeline.pViewportState(&vp);
    pipeline.pDepthStencilState(&ds);
    pipeline.pStages(shaderStages);
    pipeline.stageCount(2);
    pipeline.renderPass(render_pass);
    pipeline.subpass(0);
    auto res = vk::createGraphicsPipelines(device, pipelineCache, 1, &pipeline, NULL, &this->pipeline);
    Q_ASSERT(res == vk::Result::eVkSuccess);
}

void VKWindow::init_sampler(VkSampler &sampler) {
    VkResult res;
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE,
        samplerCreateInfo.maxAnisotropy = 0;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    /* create sampler */
    res = vkCreateSampler(device, &samplerCreateInfo, NULL, &sampler);
    assert(res == VK_SUCCESS);
}

void VKWindow::init_viewports() {
    viewport.height = (float)height;
    viewport.width = (float)width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(cmd, 0, NUM_VIEWPORTS, &viewport);
}

void VKWindow::init_scissors() {
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd, 0, NUM_SCISSORS, &scissor);
}

void VKWindow::init_fence(VkFence &fence) {
    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(device, &fenceInfo, NULL, &fence);
}

void VKWindow::init_submit_info(VkSubmitInfo &submit_info,
    VkPipelineStageFlags &pipe_stage_flags) {
    submit_info.pNext = NULL;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &presentCompleteSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;
}

void VKWindow::init_present_info(VkPresentInfoKHR &present) {
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &swap_chain;
    present.pImageIndices = &current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;
}

void VKWindow::init_clear_color_and_depth(VkClearValue *clear_values) {
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
}

void VKWindow::init_render_pass_begin_info(VkRenderPassBeginInfo &rp_begin) {
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = render_pass;
    rp_begin.framebuffer = framebuffers[current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = width;
    rp_begin.renderArea.extent.height = height;
    rp_begin.clearValueCount = 0;
    rp_begin.pClearValues = nullptr;
}

void VKWindow::destroy_pipeline() {
    vkDestroyPipeline(device, pipeline, NULL);
}

void VKWindow::destroy_pipeline_cache() {
    vkDestroyPipelineCache(device, pipelineCache, NULL);
}

void VKWindow::destroy_uniform_buffer() {
    vkDestroyBuffer(device, uniform_data.buf, NULL);
    vkFreeMemory(device, uniform_data.mem, NULL);
}

void VKWindow::destroy_descriptor_and_pipeline_layouts() {
    for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++)
        vkDestroyDescriptorSetLayout(device, desc_layout[i], NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
}

void VKWindow::destroy_descriptor_pool() {
    vkDestroyDescriptorPool(device, desc_pool, NULL);
}

void VKWindow::destroy_shaders() {
    vkDestroyShaderModule(device, shaderStages[0].module(), NULL);
    vkDestroyShaderModule(device, shaderStages[1].module(), NULL);
}

void VKWindow::destroy_command_buffer() {
    VkCommandBuffer cmd_bufs[1] = { cmd };
    vkFreeCommandBuffers(device, cmd_pool, 1, cmd_bufs);
}

void VKWindow::destroy_command_pool() {
    vkDestroyCommandPool(device, cmd_pool, NULL);
}

void VKWindow::destroy_depth_buffer() {
    vkDestroyImageView(device, depth.view, NULL);
    vkDestroyImage(device, depth.image, NULL);
    vkFreeMemory(device, depth.mem, NULL);
}

void VKWindow::destroy_vertex_buffer() {
    vkDestroyBuffer(device, vertex_buffer.buf, NULL);
    vkFreeMemory(device, vertex_buffer.mem, NULL);
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
    vkDestroyInstance(inst, NULL);
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
})SHADER";

static const char *fragShaderText =
R"SHADER(#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;
void main() {
   outColor = color;
})SHADER";


VKWindow::VKWindow(QObject* parent) {
    installEventFilter(new VkCloseEventFilter(this));
    const bool depthPresent = true;
    init_global_layer_properties();
    init_instance_extension_names();
    init_device_extension_names();
    init_instance("ShadertoyVR");
    init_enumerate_device();
    init_window_size(500, 500);
    init_swapchain_extension();
    init_device();

    init_command_pool();
    init_command_buffer();
    execute_begin_command_buffer();
    init_device_queue();
    init_swap_chain();
    init_depth_buffer();
    init_uniform_buffer();
    init_descriptor_and_pipeline_layouts(false);
    init_renderpass(depthPresent);
    init_shaders(vertShaderText, fragShaderText);
    init_framebuffers(depthPresent);
    init_vertex_buffer(g_vb_solid_face_colors_Data,
        sizeof(g_vb_solid_face_colors_Data),
        sizeof(g_vb_solid_face_colors_Data[0]), false);
    init_descriptor_pool(false);
    init_descriptor_set(false);
    init_pipeline_cache();
    init_pipeline(depthPresent);
    draw();
}

void VKWindow::draw() {
    /* VULKAN_KEY_START */
    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType =
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    VkResult res = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo,
        NULL, &presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
        presentCompleteSemaphore, NULL,
        &current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = render_pass;
    rp_begin.framebuffer = framebuffers[current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = width;
    rp_begin.renderArea.extent.height = height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, NUM_DESCRIPTOR_SETS,
        desc_set.data(), 0, NULL);

    const VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer.buf, offsets);

    init_viewports();
    init_scissors();

    vkCmdDraw(cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = buffers[current_buffer].image;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
        NULL, 1, &prePresentBarrier);

    res = vkEndCommandBuffer(cmd);
    const VkCommandBuffer cmd_bufs[] = { cmd };
    VkFenceCreateInfo fenceInfo;
    VkFence drawFence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(device, &fenceInfo, NULL, &drawFence);

    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &presentCompleteSemaphore;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(queue, 1, submit_info, drawFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */

    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &swap_chain;
    present.pImageIndices = &current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;

    /* Make sure command buffer is finished before presenting */
    do {
        res =
            vkWaitForFences(device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);

    assert(res == VK_SUCCESS);
    res = vkQueuePresentKHR(queue, &present);
    assert(res == VK_SUCCESS);

    /* VULKAN_KEY_END */
    vkDestroySemaphore(device, presentCompleteSemaphore, NULL);
    vkDestroyFence(device, drawFence, NULL);
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
