#define VMA_IMPLEMENTATION
#include "vulkan.h"

#include <iostream>
#include <vulkan/vulkan_format_traits.hpp>

#include "DirStackFileIncluder.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
template <typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return v < lo ? lo : hi < v ? hi : v;
}

EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage) {
    switch (stage) {
        case vk::ShaderStageFlagBits::eVertex:
            return EShLangVertex;
        case vk::ShaderStageFlagBits::eTessellationControl:
            return EShLangTessControl;
        case vk::ShaderStageFlagBits::eTessellationEvaluation:
            return EShLangTessEvaluation;
        case vk::ShaderStageFlagBits::eGeometry:
            return EShLangGeometry;
        case vk::ShaderStageFlagBits::eFragment:
            return EShLangFragment;
        case vk::ShaderStageFlagBits::eCompute:
            return EShLangCompute;
        case vk::ShaderStageFlagBits::eRaygenNV:
            return EShLangRayGenNV;
        case vk::ShaderStageFlagBits::eAnyHitNV:
            return EShLangAnyHitNV;
        case vk::ShaderStageFlagBits::eClosestHitNV:
            return EShLangClosestHitNV;
        case vk::ShaderStageFlagBits::eMissNV:
            return EShLangMissNV;
        case vk::ShaderStageFlagBits::eIntersectionNV:
            return EShLangIntersectNV;
        case vk::ShaderStageFlagBits::eCallableNV:
            return EShLangCallableNV;
        case vk::ShaderStageFlagBits::eTaskNV:
            return EShLangTaskNV;
        case vk::ShaderStageFlagBits::eMeshNV:
            return EShLangMeshNV;
        default:
            assert(false && "Unknown shader stage");
            return EShLangVertex;
    }
}

bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, const std::string& glslShader,
               std::vector<unsigned int>& spvShader) {
    EShLanguage stage = translateShaderStage(shaderType);

    const char* shaderStrings[1];
    shaderStrings[0] = glslShader.data();

    glslang::TShader shader(stage);
    shader.setStrings(shaderStrings, 1);

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory("shaders");

    if (!shader.parse(GetDefaultResources(), 100, false, messages, includer)) {
        std::cerr << shader.getInfoLog();
        std::cerr << shader.getInfoDebugLog();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    //
    // Program-level processing...
    //

    if (!program.link(messages)) {
        std::cerr << program.getInfoLog();
        std::cerr << program.getInfoDebugLog();
        return false;
    }

    glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
    return true;
}
}  // namespace

Vulkan::~Vulkan() {
    device.waitIdle();

    frame.destroy(device, commandPool);
    destroySwapChain();
    vmaDestroyAllocator(vmaAllocator);

    device.destroyRenderPass(renderPass);
    for (auto& pipeline : graphicsPipelines) device.destroyPipeline(pipeline);
    for (auto& pipelineLayout : pipelineLayouts) device.destroyPipelineLayout(pipelineLayout);
    for (auto& pool : descriptorPools) device.destroyDescriptorPool(pool);
    for (auto& layout : descriptorSetLayouts) device.destroyDescriptorSetLayout(layout);
    device.freeCommandBuffers(commandPool, commandBuffer);
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
Vulkan& Vulkan::setInstanceLayers(const vk::ArrayProxyNoTemporaries<const char* const>& layers) {
    instanceCreateInfo.setPEnabledLayerNames(layers);
    return *this;
}
Vulkan& Vulkan::setInstanceExtensions(const vk::ArrayProxyNoTemporaries<const char* const>& extensions) {
    instanceCreateInfo.setPEnabledExtensionNames(extensions);
    return *this;
}
Vulkan& Vulkan::setDeviceLayers(const vk::ArrayProxyNoTemporaries<const char* const>& layers) {
    deviceCreateInfo.setPEnabledLayerNames(layers);
    return *this;
}
Vulkan& Vulkan::setDeviceExtensions(const vk::ArrayProxyNoTemporaries<const char* const>& extensions) {
    deviceExtensions.insert(deviceExtensions.end(), extensions.begin(), extensions.end());
    return *this;
}
Vulkan& Vulkan::setDeviceFeatures(const vk::PhysicalDeviceFeatures& features) {
    deviceFeatures = features;
    return *this;
}
Vulkan& Vulkan::setBackgroudColor(const vk::ClearColorValue& value) {
    bgColor = value;
    return *this;
}

void Vulkan::init(vk::Extent2D extent, std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR,
                  std::function<bool(const vk::PhysicalDevice&)> pickDevice) {
    initInstance();
    enumerateDevice(pickDevice);
    initDevice(getSurfaceKHR);
    initSwapChain(extent);
    initCommandBuffer();
    initDepthBuffer();
    initRenderPass();
    initFrameBuffers();
    frame.init(device, commandPool);
};

size_t Vulkan::attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                            const Buffer& vertex, const std::vector<vk::Format>& vertexFormats,
                            const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures,
                            vk::CullModeFlags cullMode) {
    vertexBuffers.push_back(vertex);

    initDescriptorSet(uniforms, textures);
    return initPipeline(vertexShaderModule, fragmentShaderModule, vertex.stride, vertexFormats, cullMode);
}

unsigned int Vulkan::renderBegin() {
    device.waitForFences(frame.drawFence(), vk::True, std::numeric_limits<uint64_t>::max());

    auto currentBuffer =
        device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), frame.imageAcquiredSemaphore());
    if (currentBuffer.result == vk::Result::eErrorOutOfDateKHR) {
        throw std::runtime_error("resize");
    } else if (currentBuffer.result != vk::Result::eSuccess && currentBuffer.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("fail to acquire swap chain image!");
    }
    assert(currentBuffer.value < framebuffers.size());

    device.resetFences(frame.drawFence());

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = bgColor;
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassBeginInfo(renderPass, framebuffers[currentBuffer.value],
                                                vk::Rect2D(vk::Offset2D(0, 0), imageExtent), clearValues);

    frame.commandBuffer().begin(vk::CommandBufferBeginInfo());
    frame.commandBuffer().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    frame.commandBuffer().setViewport(0, vk::Viewport(0, 0, imageExtent.width, imageExtent.height, 0, 1));
    frame.commandBuffer().setScissor(0, vk::Rect2D({0, 0}, imageExtent));

    return currentBuffer.value;
}

void Vulkan::updateVertex(size_t i, const Buffer& vertex) { vertexBuffers[i] = vertex; }

void Vulkan::draw(size_t i) {
    frame.commandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipelines[i]);
    frame.commandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts[i], 0, descriptorSets[i],
                                             nullptr);
    frame.commandBuffer().bindVertexBuffers(0, vertexBuffers[i].buffer, {0});
    frame.commandBuffer().draw(vertexBuffers[i].size / vertexBuffers[i].stride, 1, 0, 0);
}

void Vulkan::renderEnd(unsigned int currentBuffer) {
    frame.commandBuffer().endRenderPass();
    frame.commandBuffer().end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    graphicsQueue.submit(vk::SubmitInfo(frame.imageAcquiredSemaphore(), waitDestinationStageMask, frame.commandBuffer(),
                                        frame.imageRenderedSemaphore()),
                         frame.drawFence());

    auto result = presentationQueue.presentKHR({frame.imageRenderedSemaphore(), swapChain, currentBuffer});
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("resize");
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    frame.next();
}

void Vulkan::render() {
    auto currentBuffer = renderBegin();
    for (size_t i = 0; i < graphicsPipelines.size(); ++i) draw(i);
    renderEnd(currentBuffer);
}

void Vulkan::resize(vk::Extent2D extent) {
    device.waitIdle();
    destroySwapChain();
    initSwapChain(extent);
    initDepthBuffer();
    initFrameBuffers();
}

Vulkan::Buffer Vulkan::createUniformBuffer(vk::DeviceSize size) {
    Buffer buffer;
    std::tie(buffer.buffer, buffer.memory) =
        createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
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

Vulkan::Buffer Vulkan::createVertexBuffer(const void* vertices, size_t stride, size_t size) {
    Buffer buffer;
    buffer.stride = stride;
    buffer.size = stride * size;

    std::tie(buffer.buffer, buffer.memory) =
        createBuffer(buffer.size, vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
    memcpy(buffer.data, vertices, buffer.size);
    vmaUnmapMemory(vmaAllocator, buffer.memory);

    return buffer;
}

void Vulkan::destroyVertexBuffer(const Buffer& buffer) {
    if (buffer.size) vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
}

Vulkan::Texture Vulkan::createTexture(vk::Extent2D extent, const void* data, uint32_t layers, bool anisotropy) {
    Texture texture;

    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    vk::FormatProperties formatProps = physicalDevice.getFormatProperties(format);
    uint32_t mipLevels = std::floor(std::log2(std::max(extent.width, extent.height))) + 1;

    bool needStaging = !(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) || (layers > 1);
    if (needStaging) {
        assert(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc);
        assert(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst);
        assert(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
    } else {
        assert(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc);
        assert(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst);
        assert(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
    }

    std::tie(texture.image, texture.memory) = createImage(
        extent, format, needStaging ? vk::ImageTiling::eOptimal : vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
        needStaging ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePreinitialized, mipLevels, layers);

    if (needStaging)
        std::tie(texture.stagingBuffer, texture.stagingMemory) =
            createBuffer(extent.width * extent.height * layers * 4, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* textureData;
    vmaMapMemory(vmaAllocator, needStaging ? texture.stagingMemory : texture.memory, &textureData);
    memcpy(textureData, data, extent.width * extent.height * layers * 4);
    vmaUnmapMemory(vmaAllocator, needStaging ? texture.stagingMemory : texture.memory);

    commandBuffer.begin(vk::CommandBufferBeginInfo());
    if (needStaging) {
        setImageLayout(commandBuffer, texture.image, format, vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal);
        vk::BufferImageCopy copyRegion(0, extent.width, extent.height, {vk::ImageAspectFlagBits::eColor, 0, 0, layers},
                                       vk::Offset3D(0, 0, 0), vk::Extent3D(extent, 1));
        commandBuffer.copyBufferToImage(texture.stagingBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal,
                                        copyRegion);
        setImageLayout(commandBuffer, texture.image, format, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal);
    } else {
        setImageLayout(commandBuffer, texture.image, format, vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferSrcOptimal);
    }
    for (uint32_t i = 1; i < mipLevels; ++i) {
        vk::ImageBlit imageBlit({vk::ImageAspectFlagBits::eColor, i - 1, 0, layers},
                                {{{}, {int32_t(extent.width >> (i - 1)), int32_t(extent.height >> (i - 1)), 1}}},
                                {vk::ImageAspectFlagBits::eColor, i, 0, layers},
                                {{{}, {int32_t(extent.width >> i), int32_t(extent.height >> i), 1}}});
        setImageLayout(commandBuffer, texture.image, format,
                       needStaging ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferDstOptimal, i, layers);
        commandBuffer.blitImage(texture.image, vk::ImageLayout::eTransferSrcOptimal, texture.image,
                                vk::ImageLayout::eTransferDstOptimal, imageBlit, vk::Filter::eLinear);
        setImageLayout(commandBuffer, texture.image, format, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal, i, layers);
    }
    setImageLayout(commandBuffer, texture.image, format, vk::ImageLayout::eTransferSrcOptimal,
                   vk::ImageLayout::eShaderReadOnlyOptimal, 0, layers, mipLevels);
    commandBuffer.end();

    vk::Fence fence = device.createFence({});
    graphicsQueue.submit(vk::SubmitInfo({}, {}, commandBuffer), fence);
    device.waitForFences(fence, vk::True, std::numeric_limits<uint64_t>::max());
    device.destroyFence(fence);

    vk::SamplerCreateInfo samplerCreateInfo(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0f, anisotropy,
        physicalDevice.getProperties().limits.maxSamplerAnisotropy, false, vk::CompareOp::eNever, 0.0f, mipLevels,
        vk::BorderColor::eFloatOpaqueBlack);
    texture.sampler = device.createSampler(samplerCreateInfo);

    vk::ImageViewCreateInfo imageViewCreateInfo({}, texture.image,
                                                layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray,
                                                format, {}, {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, layers});
    texture.view = device.createImageView(imageViewCreateInfo);

    return texture;
}

void Vulkan::destroyTexture(const Texture& texture) {
    device.destroy(texture.view);
    device.destroySampler(texture.sampler);
    vmaDestroyBuffer(vmaAllocator, texture.stagingBuffer, texture.stagingMemory);
    vmaDestroyImage(vmaAllocator, texture.image, texture.memory);
}

vk::ShaderModule Vulkan::createShaderModule(vk::ShaderStageFlagBits shaderStage, const std::string& shaderText) {
    std::vector<unsigned int> shaderSPV;
    if (!GLSLtoSPV(shaderStage, shaderText, shaderSPV)) {
        throw std::runtime_error("Could not covert glsl shader to SPIRV");
    }

    return device.createShaderModule({{}, shaderSPV});
}

void Vulkan::destroyShaderModule(const vk::ShaderModule& shader) { device.destroyShaderModule(shader); }

void Vulkan::initInstance() {
    static vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr =
        dl.template getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

    if (!applicationInfo.apiVersion) applicationInfo.apiVersion = VK_API_VERSION_1_0;
    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
    instance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void Vulkan::enumerateDevice(std::function<bool(const vk::PhysicalDevice&)> pickDevice) {
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) throw std::runtime_error("No vulkan device detected!");

    physicalDevice = physicalDevices[0];

    if (pickDevice) {
        auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pickDevice);
        if (it != physicalDevices.end()) physicalDevice = *it;
    }
}

void Vulkan::initDevice(std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR) {
    surface = getSurfaceKHR(instance);
    if (!surface) throw std::runtime_error("Cannot get surface!");

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // we want that queue support both graphics and compute
    auto it = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                           [](const vk::QueueFamilyProperties& qfp) {
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
            break;
        }
    }
    if (presentationQueueFamliyIndex >= queueFamilyProperties.size())
        throw std::runtime_error("Cannot get presentation queue!");

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfos[2];
    deviceQueueCreateInfos[0].setQueueCount(1);
    deviceQueueCreateInfos[0].setQueueFamilyIndex(graphicsQueueFamliyIndex);
    deviceQueueCreateInfos[0].setPQueuePriorities(&queuePriority);
    if (graphicsQueueFamliyIndex == presentationQueueFamliyIndex) {
        deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfos[0]);
    } else {
        deviceQueueCreateInfos[1].setQueueCount(1);
        deviceQueueCreateInfos[1].setQueueFamilyIndex(presentationQueueFamliyIndex);
        deviceQueueCreateInfos[1].setPQueuePriorities(&queuePriority);
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

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
}

void Vulkan::initCommandBuffer() {
    commandPool =
        device.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamliyIndex});

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    device.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer);
}

void Vulkan::initSwapChain(vk::Extent2D extent) {
    vk::SurfaceFormatKHR surfaceFormat = pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));

    auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    if (surfaceCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        imageExtent.width = clamp(extent.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
        imageExtent.height = clamp(extent.height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
    } else {
        imageExtent = surfaceCaps.currentExtent;
    }

    auto preTransform = surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity
                            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                            : surfaceCaps.currentTransform;

    auto compositeAlpha = surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
                              ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
                          : surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
                              ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
                          : surfaceCaps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit
                              ? vk::CompositeAlphaFlagBitsKHR::eInherit
                              : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    auto presentMode = std::find_if(presentModes.begin(), presentModes.end(),
                                    [](const vk::PresentModeKHR& mode) {
                                        return mode == vk::PresentModeKHR::eMailbox;
                                    }) != presentModes.end()
                           ? vk::PresentModeKHR::eMailbox
                           : vk::PresentModeKHR::eFifo;

    imageCount = clamp(3u, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        {}, surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, imageExtent, 1,
        vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, graphicsQueueFamliyIndex, preTransform,
        compositeAlpha, presentMode, true);

    auto queueFamilyIndices = {graphicsQueueFamliyIndex, presentationQueueFamliyIndex};
    if (graphicsQueueFamliyIndex != presentationQueueFamliyIndex) {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.setQueueFamilyIndices(queueFamilyIndices);
    }

    swapChain = device.createSwapchainKHR(swapChainCreateInfo);

    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageViews.reserve(swapChainImages.size());
    vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, surfaceFormat.format, {},
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

    std::tie(depthImage, depthMemory) =
        createImage(imageExtent, depthFormat, tiling,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment);
    depthImageView = device.createImageView(
        {{}, depthImage, vk::ImageViewType::e2D, depthFormat, {}, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});
}

void Vulkan::initDescriptorSet(const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures) {
    std::array<vk::DescriptorPoolSize, 2> poolSizes;
    poolSizes[0] = {vk::DescriptorType::eUniformBuffer, (uint32_t)uniforms.size()};
    poolSizes[1] = {vk::DescriptorType::eCombinedImageSampler, (uint32_t)textures.size()};
    descriptorPools.push_back(device.createDescriptorPool({{}, 1, poolSizes}));

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    descriptorSetLayoutBindings.reserve(uniforms.size() + textures.size());
    for (auto uniform : uniforms)
        descriptorSetLayoutBindings.emplace_back(uniform.first, vk::DescriptorType::eUniformBuffer, 1,
                                                 uniform.second.stage);
    for (auto texture : textures)
        descriptorSetLayoutBindings.emplace_back(texture.first, vk::DescriptorType::eCombinedImageSampler, 1,
                                                 texture.second.stage);
    descriptorSetLayouts.push_back(device.createDescriptorSetLayout({{}, descriptorSetLayoutBindings}));

    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPools.back(), 1, &descriptorSetLayouts.back());
    device.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet);
    descriptorSets.push_back(descriptorSet);

    std::vector<vk::DescriptorBufferInfo> descriptorBufferInfo;
    descriptorBufferInfo.reserve(uniforms.size());
    std::vector<vk::DescriptorImageInfo> descriptorImageInfo;
    descriptorImageInfo.reserve(textures.size());
    std::vector<vk::WriteDescriptorSet> writeDescriptorSet;
    writeDescriptorSet.reserve(uniforms.size() + textures.size());

    for (const auto& uniform : uniforms) {
        descriptorBufferInfo.emplace_back(uniform.second.buffer, 0, uniform.second.size);
        writeDescriptorSet.emplace_back(descriptorSets.back(), uniform.first, 0, 1, vk::DescriptorType::eUniformBuffer,
                                        nullptr, &descriptorBufferInfo.back());
    }
    for (const auto& texture : textures) {
        descriptorImageInfo.emplace_back(texture.second.sampler, texture.second.view,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
        writeDescriptorSet.emplace_back(descriptorSets.back(), texture.first, 0, 1,
                                        vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo.back());
    }

    device.updateDescriptorSets(writeDescriptorSet, nullptr);
}

void Vulkan::initRenderPass() {
    vk::Format colorFormat = pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface)).format;

    std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
    attachmentDescriptions[0] = vk::AttachmentDescription(
        {}, colorFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);
    attachmentDescriptions[1] = vk::AttachmentDescription(
        {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);

    renderPass = device.createRenderPass({{}, attachmentDescriptions, subpass});
}

void Vulkan::initFrameBuffers() {
    std::array<vk::ImageView, 2> attachments;
    attachments[1] = depthImageView;

    vk::FramebufferCreateInfo framebufferCreateInfo({}, renderPass, attachments, imageExtent.width, imageExtent.height,
                                                    1);

    framebuffers.reserve(swapChainImageViews.size());
    for (auto const& imageView : swapChainImageViews) {
        attachments[0] = imageView;
        framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo));
    }
}

size_t Vulkan::initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                            uint32_t vertexStride, const std::vector<vk::Format>& vertexFormats,
                            vk::CullModeFlags cullMode, bool depthBuffered) {
    std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main")};

    std::vector<vk::VertexInputAttributeDescription> vertexInputAtrributeDescription;
    vk::VertexInputBindingDescription vertexInputBindingDescription(0, vertexStride);
    vk::PipelineVertexInputStateCreateInfo pipelineVertexInputeStateCreateInfo;

    if (vertexStride > 0) {
        vertexInputAtrributeDescription.reserve(vertexFormats.size());
        uint32_t offset = 0;
        for (uint32_t i = 0; i < vertexFormats.size(); ++i) {
            vertexInputAtrributeDescription.emplace_back(i, 0, vertexFormats[i], offset);
            offset += vk::blockSize(vertexFormats[i]);
        }
        pipelineVertexInputeStateCreateInfo.setVertexBindingDescriptions(vertexInputBindingDescription);
        pipelineVertexInputeStateCreateInfo.setVertexAttributeDescriptions(vertexInputAtrributeDescription);
    }

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo({},
                                                                                  vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport(0, 0, imageExtent.width, imageExtent.height, 0, 1);
    vk::Rect2D scissor({0, 0}, imageExtent);
    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo({}, viewport, scissor);

    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
        {}, false, false, vk::PolygonMode::eFill, cullMode, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f,
        1.0f);
    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo({}, depthBuffered, depthBuffered,
                                                                                vk::CompareOp::eLessOrEqual);

    vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
        true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne,
        vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eNoOp,
                                                                            pipelineColorBlendAttachmentState);

    std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo({}, dynamicStates);

    vk::PipelineLayout pipelineLayout = device.createPipelineLayout({{}, descriptorSetLayouts.back()});
    pipelineLayouts.push_back(pipelineLayout);

    vk::GraphicsPipelineCreateInfo graphicPipelineCreateInfo(
        {}, pipelineShaderStageCreateInfos, &pipelineVertexInputeStateCreateInfo, &pipelineInputAssemblyStateCreateInfo,
        nullptr, &pipelineViewportStateCreateInfo, &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo, &pipelineDepthStencilStateCreateInfo, &pipelineColorBlendStateCreateInfo,
        &pipelineDynamicStateCreateInfo, pipelineLayout, renderPass);

    vk::Result result;
    vk::Pipeline pipeline;
    std::tie(result, pipeline) = device.createGraphicsPipeline(nullptr, graphicPipelineCreateInfo);
    assert(result == vk::Result::eSuccess);

    graphicsPipelines.push_back(pipeline);
    return graphicsPipelines.size() - 1;
}

void Vulkan::destroySwapChain() {
    for (auto const& framebuffer : framebuffers) device.destroyFramebuffer(framebuffer);
    framebuffers.clear();
    for (auto& swapChainImageView : swapChainImageViews) device.destroyImageView(swapChainImageView);
    swapChainImageViews.clear();
    device.destroySwapchainKHR(swapChain);

    device.destroyImageView(depthImageView);
    vmaDestroyImage(vmaAllocator, depthImage, depthMemory);
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
        for (auto& requestedFormat : {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8Unorm,
                                      vk::Format::eB8G8R8Unorm}) {
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

std::pair<vk::Buffer, VmaAllocation> Vulkan::createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage,
                                                          vk::MemoryPropertyFlags bufferProp) {
    vk::BufferCreateInfo bufferCreateInfo({}, bufferSize, bufferUsage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (bufferProp) {
        if (bufferProp & vk::MemoryPropertyFlagBits::eHostVisible)
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(bufferProp);
    }

    vk::Buffer buffer;
    VmaAllocation bufferMemory;
    vmaCreateBuffer(vmaAllocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocInfo,
                    reinterpret_cast<VkBuffer*>(&buffer), &bufferMemory, nullptr);

    return {buffer, bufferMemory};
}

std::pair<vk::Image, VmaAllocation> Vulkan::createImage(vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
                                                        vk::ImageUsageFlags usage, vk::ImageLayout layout,
                                                        uint32_t mipLevels, uint32_t layers) {
    vk::ImageCreateInfo imageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(extent, 1), mipLevels, layers,
                                        vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive, {},
                                        layout);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (usage & vk::ImageUsageFlagBits::eTransientAttachment)
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    else if (layout == vk::ImageLayout::ePreinitialized)
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    vk::Image image;
    VmaAllocation imageMemory;
    vmaCreateImage(vmaAllocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocInfo,
                   reinterpret_cast<VkImage*>(&image), &imageMemory, nullptr);

    return {image, imageMemory};
}

void Vulkan::setImageLayout(const vk::CommandBuffer& commandBuffer, vk::Image image, vk::Format format,
                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel,
                            uint32_t layerCount, uint32_t mipCount) {
    vk::AccessFlags sourceAccessMask;
    switch (oldLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            sourceAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::ePreinitialized:
            sourceAccessMask = vk::AccessFlagBits::eHostWrite;
            break;
        case vk::ImageLayout::eGeneral:  // sourceAccessMask is empty
            [[fallthrough]];
        case vk::ImageLayout::eUndefined:
            break;
        default:
            assert(false);
            break;
    }

    vk::PipelineStageFlags sourceStage;
    switch (oldLayout) {
        case vk::ImageLayout::eGeneral:
            [[fallthrough]];
        case vk::ImageLayout::ePreinitialized:
            sourceStage = vk::PipelineStageFlagBits::eHost;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            [[fallthrough]];
        case vk::ImageLayout::eTransferDstOptimal:
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            break;
        case vk::ImageLayout::eUndefined:
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            break;
        default:
            assert(false);
            break;
    }

    vk::AccessFlags destinationAccessMask;
    switch (newLayout) {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eGeneral:  // empty destinationAccessMask
            [[fallthrough]];
        case vk::ImageLayout::ePresentSrcKHR:
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        default:
            assert(false);
            break;
    }

    vk::PipelineStageFlags destinationStage;
    switch (newLayout) {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            break;
        case vk::ImageLayout::eGeneral:
            destinationStage = vk::PipelineStageFlagBits::eHost;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            [[fallthrough]];
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
            break;
        default:
            assert(false);
            break;
    }

    vk::ImageAspectFlags aspectMask;
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint) {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask, destinationAccessMask, oldLayout, newLayout,
                                              vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, image,
                                              {aspectMask, mipLevel, mipCount, 0, layerCount});
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
}
