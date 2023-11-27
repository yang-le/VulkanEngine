#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <initializer_list>
#include <limits>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>


class Vulkan {
public:
    Vulkan() = default;
    ~Vulkan() {
        device.destroyRenderPass(renderPass);
        device.freeDescriptorSets(descriptorPool, descriptorSets);
        device.destroyDescriptorPool(descriptorPool);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyDescriptorSetLayout(descriptorSetLayout);

        vmaUnmapMemory(vmaAllocator, uniformDataMemory);
        vmaDestroyBuffer(vmaAllocator, uniformDataBuffer, uniformDataMemory);
        vmaDestroyAllocator(vmaAllocator);

        for (auto& imageView : swapChainImageViews)
            device.destroyImageView(imageView);
        device.destroySwapchainKHR(swapChain);
        if (!commandBuffers.empty())
            device.freeCommandBuffers(commandPool, commandBuffers);
        if (commandPool)
            device.destroyCommandPool(commandPool);
        if (device)
            device.destroy();

        instance.destroySurfaceKHR(surface);
        if (instance)
            instance.destroy();
    }

    operator VkInstance() const {
        return instance;
    }

public:
    Vulkan& setAppInfo(const char* appName, uint32_t appVerson = {}) {
        applicationInfo.setPApplicationName(appName);
        applicationInfo.setApplicationVersion(appVerson);
        return *this;
    }
    Vulkan& setEngineInfo(const char* engineName, uint32_t engineVersion = {}) {
        applicationInfo.setPEngineName(engineName);
        applicationInfo.setEngineVersion(engineVersion);
        return *this;
    }
    Vulkan& setApiVersion(uint32_t apiVersion) {
        applicationInfo.setApiVersion(apiVersion);
        return *this;
    }
    Vulkan& setInstanceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers) {
        instanceCreateInfo.setPEnabledLayerNames(layers);
        return *this;
    }
    Vulkan& setInstanceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions) {
        instanceCreateInfo.setPEnabledExtensionNames(extensions);
        return *this;
    }
    Vulkan& setDeviceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers) {
        deviceCreateInfo.setPEnabledLayerNames(layers);
        return *this;
    }
    Vulkan& setDeviceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions) {
        deviceExtensions.insert(deviceExtensions.end(), extensions.begin(), extensions.end());
        return *this;
    }
    Vulkan& setDeviceFeatures(const vk::PhysicalDeviceFeatures& features) {
        deviceFeatures = features;
        return *this;
    }

    bool init(uint32_t width, uint32_t height, std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR, std::function<bool(const vk::PhysicalDevice &)> pickDevice = {}) {
        initInstance();
        if (!enumerateDevice(pickDevice))
            return false;
        if (!initDevice(getSurfaceKHR))
            return false;
        initCommandBuffer();
        if (!initSwapChain(width, height))
            return false;
        initUniformBuffer();
        initPipelineLayout();
        initDescriptorSet();
        initRenderPass();

        return true;
    }

private:
    void initInstance() {
        static vk::DynamicLoader dl;
        PFN_vkGetInstanceProcAddr getInstanceProcAddr = dl.template getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

        instanceCreateInfo.setPApplicationInfo(&applicationInfo);
        instance = vk::createInstance(instanceCreateInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    }

    bool enumerateDevice(std::function<bool(const vk::PhysicalDevice &)> pickDevice) {
        auto physicalDevices = instance.enumeratePhysicalDevices();
        if (physicalDevices.empty())
            return false;

        physicalDevice = physicalDevices.front();

        if (pickDevice) {
            auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pickDevice);
            if (it != physicalDevices.end())
                physicalDevice = *it;
        }

        return true;
    }

    bool initDevice(std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR) {
        surface = getSurfaceKHR(instance);
        if (!surface)
            return false;

        auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // we want that queue support both graphics and compute
        auto it = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const vk::QueueFamilyProperties& qfp) {
            return qfp.queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics);
        });
        graphicsQueueFamliyIndex = std::distance(queueFamilyProperties.begin(), it);
        if (graphicsQueueFamliyIndex >= queueFamilyProperties.size())
            return false;

        // get presentation supported queue
        presentationQueueFamliyIndex = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
            if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
                presentationQueueFamliyIndex = i;
            }
        }
        if (presentationQueueFamliyIndex >= queueFamilyProperties.size())
            return false;

        if (graphicsQueueFamliyIndex == presentationQueueFamliyIndex) {
            vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, graphicsQueueFamliyIndex);
            deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
        } else {
            vk::DeviceQueueCreateInfo deviceQueueCreateInfos[2];
            deviceQueueCreateInfos[0].setQueueFamilyIndex(graphicsQueueFamliyIndex);
            deviceQueueCreateInfos[1].setQueueFamilyIndex(presentationQueueFamliyIndex);
            deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfos);
        }

        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);

        device = physicalDevice.createDevice(deviceCreateInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

        graphicsQueue = device.getQueue(graphicsQueueFamliyIndex, 0);
        presentationQueue = device.getQueue(presentationQueueFamliyIndex, 0);

        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.physicalDevice         = physicalDevice;
        allocatorCreateInfo.device                 = device;
        allocatorCreateInfo.instance               = instance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);

        return true;
    }

    void initCommandBuffer() {
        commandPool = device.createCommandPool({{}, graphicsQueueFamliyIndex});
        commandBuffers = device.allocateCommandBuffers({commandPool, vk::CommandBufferLevel::ePrimary, 1});
    }

    bool initSwapChain(uint32_t width, uint32_t height) {
        auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
        if (formats.empty())
            return false;

        vk::Format format = formats[0].format == vk::Format::eUndefined ? vk::Format::eR8G8B8A8Unorm : formats[0].format;

        auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

        vk::Extent2D imageExtent;
        if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
            imageExtent.width = std::clamp(width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
            imageExtent.height = std::clamp(height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
        } else {
            imageExtent = surfaceCaps.currentExtent;
        }

        auto preTransform =
            surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCaps.currentTransform;

        auto compositeAlpha =
            surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
            : surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
            : surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit ? vk::CompositeAlphaFlagBitsKHR::eInherit
            : vk::CompositeAlphaFlagBitsKHR::eOpaque;

        auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        auto presentMode = std::find_if(presentModes.begin(), presentModes.end(), [](const vk::PresentModeKHR& mode) {
            return mode == vk::PresentModeKHR::eMailbox;
        }) != presentModes.end() ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo({}, surface,
            std::clamp(3u, surfaceCaps.minImageCount, surfaceCaps.maxImageCount),
            format, vk::ColorSpaceKHR::eSrgbNonlinear, imageExtent, 1,
            vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, {},
            preTransform, compositeAlpha, presentMode, true
        );
        if (graphicsQueueFamliyIndex != presentationQueueFamliyIndex) {
            swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            auto queueFamilyIndices = {graphicsQueueFamliyIndex, presentationQueueFamliyIndex};
            swapChainCreateInfo.setQueueFamilyIndices(queueFamilyIndices);
        }

        swapChain = device.createSwapchainKHR(swapChainCreateInfo);

        swapChainImages = device.getSwapchainImagesKHR(swapChain);
        swapChainImageViews.reserve(swapChainImages.size());
        vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, format, {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        for (auto image : swapChainImages) {
            imageViewCreateInfo.image = image;
            swapChainImageViews.push_back(device.createImageView(imageViewCreateInfo));
        }

        return true;
    }

    void initUniformBuffer() {
        std::tie(uniformDataBuffer, uniformDataMemory) = createBuffer(sizeof(glm::mat4), vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        vmaMapMemory(vmaAllocator, uniformDataMemory, &uniformDataMapped);
    }

    void initPipelineLayout() {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
        descriptorSetLayout = device.createDescriptorSetLayout({{}, descriptorSetLayoutBinding});

        pipelineLayout = device.createPipelineLayout({{}, descriptorSetLayout});
    }

    void initDescriptorSet() {
        vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, 1);
        descriptorPool = device.createDescriptorPool({{}, 1, poolSize});

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, descriptorSetLayout);
        descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

        vk::DescriptorBufferInfo descriptorBufferInfo(uniformDataBuffer, 0, sizeof(glm::mat4));
        vk::WriteDescriptorSet writeDescriptorSet(descriptorSets.front(), 0, 0, vk::DescriptorType::eUniformBuffer, {}, descriptorBufferInfo);
        device.updateDescriptorSets(writeDescriptorSet, nullptr);
    }

    void initRenderPass() {
        vk::Format colorFormat = pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface)).format;

        std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
        attachmentDescriptions[0] = vk::AttachmentDescription({}, colorFormat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
        attachmentDescriptions[1] = vk::AttachmentDescription({}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal);
        vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);

        renderPass = device.createRenderPass({{}, attachmentDescriptions, subpass});
    }

    //
    // Utils
    //

    vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
        assert(!formats.empty());
        vk::SurfaceFormatKHR pickedFormat = formats[0];
        if (formats.size() == 1) {
            if (formats[0].format == vk::Format::eUndefined) {
                pickedFormat.format = vk::Format::eR8G8B8A8Unorm;
                pickedFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        } else {
            for (auto& requestedFormat : {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8Unorm, vk::Format::eB8G8R8Unorm}) {
                auto it = std::find_if(formats.begin(), formats.end(), [requestedFormat](const vk::SurfaceFormatKHR& f) {
                    return f.format == requestedFormat && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                });
                if (it != formats.end()) {
                    pickedFormat = *it;
                    break;
                }
            }
        }
        assert(pickedFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
        return pickedFormat;
    }

    std::pair<vk::Buffer, VmaAllocation> createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags bufferProp) {
        vk::BufferCreateInfo bufferCreateInfo({}, bufferSize, bufferUsage);

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(bufferProp);

        vk::Buffer buffer;
        VmaAllocation bufferMemory;
        vmaCreateBuffer(vmaAllocator, reinterpret_cast<const VkBufferCreateInfo *>(&bufferCreateInfo), &allocInfo, reinterpret_cast<VkBuffer*>(&buffer), &bufferMemory, nullptr);

        return {buffer, bufferMemory};
    }

private:
    vk::ApplicationInfo applicationInfo;
    vk::InstanceCreateInfo instanceCreateInfo;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::DeviceCreateInfo deviceCreateInfo;
    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamliyIndex;
    uint32_t presentationQueueFamliyIndex;
    std::vector<const char*> deviceExtensions;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentationQueue;
    #define computeQueue graphicsQueue
    VmaAllocator vmaAllocator;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::PipelineLayout pipelineLayout;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::Buffer uniformDataBuffer;
    VmaAllocation uniformDataMemory;
    void* uniformDataMapped;
    vk::RenderPass renderPass;
};
