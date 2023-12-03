#define VMA_IMPLEMENTATION
#include "vulkan.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
    template <typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
        return v < lo ? lo : hi < v ? hi : v;
    }

    EShLanguage translateShaderStage( vk::ShaderStageFlagBits stage ) {
      switch ( stage ) {
        case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
        case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
        case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
        case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
        case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
        case vk::ShaderStageFlagBits::eRaygenNV: return EShLangRayGenNV;
        case vk::ShaderStageFlagBits::eAnyHitNV: return EShLangAnyHitNV;
        case vk::ShaderStageFlagBits::eClosestHitNV: return EShLangClosestHitNV;
        case vk::ShaderStageFlagBits::eMissNV: return EShLangMissNV;
        case vk::ShaderStageFlagBits::eIntersectionNV: return EShLangIntersectNV;
        case vk::ShaderStageFlagBits::eCallableNV: return EShLangCallableNV;
        case vk::ShaderStageFlagBits::eTaskNV: return EShLangTaskNV;
        case vk::ShaderStageFlagBits::eMeshNV: return EShLangMeshNV;
        default: assert( false && "Unknown shader stage" ); return EShLangVertex;
      }
    }

    bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, const std::string& glslShader, std::vector<unsigned int>& spvShader) {
        EShLanguage stage = translateShaderStage(shaderType);

        const char* shaderStrings[1];
        shaderStrings[0] = glslShader.data();

        glslang::TShader shader(stage);
        shader.setStrings(shaderStrings, 1);

        // Enable SPIR-V and Vulkan rules when parsing GLSL
        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

        if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
            puts(shader.getInfoLog());
            puts(shader.getInfoDebugLog());
            return false;
        }

        glslang::TProgram program;
        program.addShader(&shader);

        //
        // Program-level processing...
        //

        if (!program.link(messages)) {
            puts(program.getInfoLog());
            puts(program.getInfoDebugLog());
            return false;
        }

        glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
        return true;
    }
}

Vulkan::~Vulkan() {
    device.waitIdle();
    device.destroyFence(drawFence);
    device.destroySemaphore(imageAcquiredSemaphore);
    device.destroyPipeline(graphicsPipeline);
    for(auto const& framebuffer : framebuffers)
        device.destroyFramebuffer(framebuffer);

    device.destroyRenderPass(renderPass);
    device.freeDescriptorSets(descriptorPool, descriptorSets);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyDescriptorSetLayout(descriptorSetLayout);

    device.destroyImageView(depthImageView);
    vmaDestroyImage(vmaAllocator, depthImage, depthMemory);
    vmaDestroyAllocator(vmaAllocator);

    for (auto& swapChainImageView : swapChainImageViews)
        device.destroyImageView(swapChainImageView);
    device.destroySwapchainKHR(swapChain);
    device.freeCommandBuffers(commandPool, commandBuffers);
    device.destroyCommandPool(commandPool);
    device.destroy();

    instance.destroySurfaceKHR(surface);
    instance.destroy();
}

Vulkan& Vulkan::setAppInfo(const char* appName, uint32_t appVerson) {
    applicationInfo.setPApplicationName(appName);
    applicationInfo.setApplicationVersion(appVerson);
    return *this;
}
Vulkan& Vulkan::setEngineInfo(const char* engineName, uint32_t engineVersion) {
    applicationInfo.setPEngineName(engineName);
    applicationInfo.setEngineVersion(engineVersion);
    return *this;
}
Vulkan& Vulkan::setApiVersion(uint32_t apiVersion) {
    applicationInfo.setApiVersion(apiVersion);
    return *this;
}
Vulkan& Vulkan::setInstanceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers) {
    instanceCreateInfo.setPEnabledLayerNames(layers);
    return *this;
}
Vulkan& Vulkan::setInstanceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions) {
    instanceCreateInfo.setPEnabledExtensionNames(extensions);
    return *this;
}
Vulkan& Vulkan::setDeviceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers) {
    deviceCreateInfo.setPEnabledLayerNames(layers);
    return *this;
}
Vulkan& Vulkan::setDeviceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions) {
    deviceExtensions.insert(deviceExtensions.end(), extensions.begin(), extensions.end());
    return *this;
}
Vulkan& Vulkan::setDeviceFeatures(const vk::PhysicalDeviceFeatures& features) {
    deviceFeatures = features;
    return *this;
}

void Vulkan::init(uint32_t width, uint32_t height, std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR, std::function<bool(const vk::PhysicalDevice &)> pickDevice) {
    initInstance();
    enumerateDevice(pickDevice);
    initDevice(getSurfaceKHR);
    initCommandBuffer();
    initSwapChain(width, height);
    initDepthBuffer();
    initPipelineLayout();
    initRenderPass();
    initFrameBuffers();

    imageAcquiredSemaphore = device.createSemaphore({});
    drawFence = device.createFence({});
};

void Vulkan::attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule, const Buffer& vertex, const Buffer& uniform, const std::vector<std::pair<vk::Format, uint32_t>>& vertexInputeAttributeFormatOffset) {
    vertexBuffer = vertex;
    uniformBuffer = uniform;

    initDescriptorSet(uniformBuffer.size);
    initPipeline(vertexShaderModule, fragmentShaderModule, vertexBuffer.stride, vertexInputeAttributeFormatOffset);
}

void Vulkan::draw() {
    auto currentBuffer = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAcquiredSemaphore);
    assert(currentBuffer.result == vk::Result::eSuccess);
    assert(currentBuffer.value < framebuffers.size());

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassBeginInfo(renderPass, framebuffers[currentBuffer.value], vk::Rect2D(vk::Offset2D(0, 0), imageExtent), clearValues);

    commandBuffers[0].begin(vk::CommandBufferBeginInfo());
    commandBuffers[0].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    commandBuffers[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets, nullptr);
    commandBuffers[0].bindVertexBuffers(0, vertexBuffer.buffer, {0});
    commandBuffers[0].draw(12 * 3, 1, 0, 0);
    commandBuffers[0].endRenderPass();
    commandBuffers[0].end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    graphicsQueue.submit(vk::SubmitInfo(imageAcquiredSemaphore, waitDestinationStageMask, commandBuffers[0]), drawFence);
    device.waitForFences(drawFence, vk::True, std::numeric_limits<uint64_t>::max());

    auto result = presentationQueue.presentKHR({{}, swapChain, currentBuffer.value});
    switch (result) {
    case vk::Result::eSuccess: break;
    case vk::Result::eSuboptimalKHR: puts("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR"); break;
    default: assert(false);
    }
}

Vulkan::Buffer Vulkan::createUniformBuffer(size_t size) {
    Buffer buffer;
    std::tie(buffer.buffer, buffer.memory) = createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    buffer.stride = buffer.size = size;
    vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);

    return buffer;
}

void Vulkan::destroyUniformBuffer(const Buffer& buffer) {
    if (buffer.size) {
        vmaUnmapMemory(vmaAllocator, buffer.memory);
        vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
    }
}

void Vulkan::destroyVertexBuffer(const Buffer& buffer) {
    if (buffer.size)
        vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
}

vk::ShaderModule Vulkan::createShaderModule(vk::ShaderStageFlagBits shaderStage, const std::string& shaderText) {
    std::vector<unsigned int> shaderSPV;
    if (!GLSLtoSPV(shaderStage, shaderText, shaderSPV)) {
        throw std::runtime_error("Could not covert glsl shader to SPIRV");
    }

    return device.createShaderModule({{}, shaderSPV});
}

void Vulkan::destroyShaderModule(const vk::ShaderModule& shader) {
    device.destroyShaderModule(shader);
}

void Vulkan::initInstance() {
    static vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = dl.template getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
    instance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void Vulkan::enumerateDevice(std::function<bool(const vk::PhysicalDevice &)> pickDevice) {
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty())
        throw std::runtime_error("No vulkan device detected!");

    physicalDevice = physicalDevices.front();

    if (pickDevice) {
        auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pickDevice);
        if (it != physicalDevices.end())
            physicalDevice = *it;
    }
}

void Vulkan::initDevice(std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR) {
    surface = getSurfaceKHR(instance);
    if (!surface)
        throw std::runtime_error("Cannot get surface!");

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // we want that queue support both graphics and compute
    auto it = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const vk::QueueFamilyProperties& qfp) {
        return qfp.queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics);
    });
    graphicsQueueFamliyIndex = std::distance(queueFamilyProperties.begin(), it);
    if (graphicsQueueFamliyIndex >= queueFamilyProperties.size())
        throw std::runtime_error("Cannot get graphics and compute queue!");

    // get presentation supported queue
    presentationQueueFamliyIndex = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
        if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
            presentationQueueFamliyIndex = i;
        }
    }
    if (presentationQueueFamliyIndex >= queueFamilyProperties.size())
        throw std::runtime_error("Cannot get presentation queue!");

    if (graphicsQueueFamliyIndex == presentationQueueFamliyIndex) {
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, graphicsQueueFamliyIndex, 1);
        deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
    } else {
        vk::DeviceQueueCreateInfo deviceQueueCreateInfos[2];
        deviceQueueCreateInfos[0].setQueueCount(1);
        deviceQueueCreateInfos[0].setQueueFamilyIndex(graphicsQueueFamliyIndex);
        deviceQueueCreateInfos[1].setQueueCount(1);
        deviceQueueCreateInfos[1].setQueueFamilyIndex(presentationQueueFamliyIndex);
        deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfos);
    }

    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

    deviceFeatures.samplerAnisotropy = vk::True;
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
}

void Vulkan::initCommandBuffer() {
    commandPool = device.createCommandPool({{}, graphicsQueueFamliyIndex});
    commandBuffers = device.allocateCommandBuffers({commandPool, vk::CommandBufferLevel::ePrimary, 1});
}

void Vulkan::initSwapChain(uint32_t width, uint32_t height) {
    auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
    if (formats.empty())
        throw std::runtime_error("No supported surface format!");

    vk::Format format = formats[0].format == vk::Format::eUndefined ? vk::Format::eR8G8B8A8Unorm : formats[0].format;

    auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        imageExtent.width = clamp(width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
        imageExtent.height = clamp(height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
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
        clamp(3u, surfaceCaps.minImageCount, surfaceCaps.maxImageCount),
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
}

void Vulkan::initDepthBuffer() {
    const vk::Format depthFormat = vk::Format::eD16Unorm;
    vk::FormatProperties formatProp = physicalDevice.getFormatProperties(depthFormat);

    vk::ImageTiling tiling;
    if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        tiling = vk::ImageTiling::eOptimal;
    } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        tiling = vk::ImageTiling::eLinear;
    } else {
        throw std::runtime_error("DepthStencilAttachment is not supported for D16Unorm depth format.");
    }

    std::tie(depthImage, depthMemory) = createImage(imageExtent.width, imageExtent.height, depthFormat, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    depthImageView = device.createImageView({{}, depthImage, vk::ImageViewType::e2D, depthFormat, {}, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});
}

void Vulkan::initPipelineLayout() {
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    descriptorSetLayout = device.createDescriptorSetLayout({{}, descriptorSetLayoutBinding});

    pipelineLayout = device.createPipelineLayout({{}, descriptorSetLayout});
}

void Vulkan::initDescriptorSet(size_t uniformBufferSize) {
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, 1);
    descriptorPool = device.createDescriptorPool({{}, 1, poolSize});

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, descriptorSetLayout);
    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

    vk::DescriptorBufferInfo descriptorBufferInfo(uniformBuffer.buffer, 0, uniformBufferSize);
    vk::WriteDescriptorSet writeDescriptorSet(descriptorSets.front(), 0, 0, vk::DescriptorType::eUniformBuffer, {}, descriptorBufferInfo);
    device.updateDescriptorSets(writeDescriptorSet, nullptr);
}

void Vulkan::initRenderPass() {
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

void Vulkan::initFrameBuffers() {
    std::array<vk::ImageView, 2> attachments;
    attachments[1] = depthImageView;

    vk::FramebufferCreateInfo framebufferCreateInfo({}, renderPass, attachments, imageExtent.width, imageExtent.height, 1);
    framebuffers.reserve(swapChainImageViews.size());
    for (auto const& imageView : swapChainImageViews) {
        attachments[0] = imageView;
        framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo));
    }
}

void Vulkan::initPipeline(
    const vk::ShaderModule& vertexShaderModule,
    const vk::ShaderModule& fragmentShaderModule,
    uint32_t vertexStride,
    const std::vector<std::pair<vk::Format, uint32_t>>& vertexInputeAttributeFormatOffset,
    bool depthBuffered) {
    std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main")
    };

    std::vector<vk::VertexInputAttributeDescription> vertexInputAtrributeDescription;
    vk::VertexInputBindingDescription vertexInputBindingDescription(0, vertexStride);
    vk::PipelineVertexInputStateCreateInfo pipelineVertexInputeStateCreateInfo;

    if (vertexStride > 0) {
        vertexInputAtrributeDescription.reserve(vertexInputeAttributeFormatOffset.size());
        for (uint32_t i = 0; i < vertexInputeAttributeFormatOffset.size(); ++i) {
            vertexInputAtrributeDescription.emplace_back(i, 0, vertexInputeAttributeFormatOffset[i].first, vertexInputeAttributeFormatOffset[i].second);
        }
        pipelineVertexInputeStateCreateInfo.setVertexBindingDescriptions(vertexInputBindingDescription);
        pipelineVertexInputeStateCreateInfo.setVertexAttributeDescriptions(vertexInputAtrributeDescription);
    }

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList);
    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo({}, false, false,
        vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
        false, 0.0f, 0.0f, 0.0f, 1.0f);
    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo({}, depthBuffered, depthBuffered, vk::CompareOp::eLessOrEqual);

    vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(false,
        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eNoOp, pipelineColorBlendAttachmentState);

    vk::GraphicsPipelineCreateInfo graphicPipelineCreateInfo({},
        pipelineShaderStageCreateInfos,
        &pipelineVertexInputeStateCreateInfo,
        &pipelineInputAssemblyStateCreateInfo,
        nullptr, nullptr,
        &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo,
        &pipelineDepthStencilStateCreateInfo,
        &pipelineColorBlendStateCreateInfo,
        nullptr,
        pipelineLayout,
        renderPass
        );

    vk::Result result;
    std::tie(result, graphicsPipeline) = device.createGraphicsPipeline(nullptr, graphicPipelineCreateInfo);
    assert(result == vk::Result::eSuccess);
}


//
// Utils
//

vk::SurfaceFormatKHR Vulkan::pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
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

std::pair<vk::Buffer, VmaAllocation> Vulkan::createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags bufferProp) {
    vk::BufferCreateInfo bufferCreateInfo({}, bufferSize, bufferUsage);

    VmaAllocationCreateInfo allocInfo = {};
    if (bufferProp) {
        allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(bufferProp);
    }

    vk::Buffer buffer;
    VmaAllocation bufferMemory;
    vmaCreateBuffer(vmaAllocator, reinterpret_cast<const VkBufferCreateInfo *>(&bufferCreateInfo), &allocInfo, reinterpret_cast<VkBuffer*>(&buffer), &bufferMemory, nullptr);

    return {buffer, bufferMemory};
}

std::pair<vk::Image, VmaAllocation> Vulkan::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage) {
    vk::ImageCreateInfo imageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(width, height, 1), 1, 1, vk::SampleCountFlagBits::e1, tiling, usage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vk::Image image;
    VmaAllocation imageMemory;
    vmaCreateImage(vmaAllocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocInfo, reinterpret_cast<VkImage*>(&image), &imageMemory, nullptr);

    return {image, imageMemory};
}
