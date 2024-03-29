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
    shader.setPreamble("#extension GL_GOOGLE_include_directive : enable\n");

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

void setImageLayout(const vk::CommandBuffer& commandBuffer, vk::Image image, vk::Format format,
                    vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel = 0,
                    uint32_t layerCount = 1, uint32_t mipCount = 1) {
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

vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
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

std::pair<vk::Buffer, VmaAllocation> createBuffer(const VmaAllocator& vmaAllocator, vk::DeviceSize bufferSize,
                                                  vk::BufferUsageFlags bufferUsage,
                                                  vk::MemoryPropertyFlags bufferProp = {}) {
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

std::pair<vk::Image, VmaAllocation> createImage(const VmaAllocator& vmaAllocator, vk::Extent2D extent,
                                                vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                                vk::ImageLayout layout = vk::ImageLayout::eUndefined,
                                                uint32_t mipLevels = 1, uint32_t layers = 1, bool cubemap = false) {
    vk::ImageCreateInfo imageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(extent, 1), mipLevels, layers,
                                        vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive, {},
                                        layout);
    if (cubemap) imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);

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

bool isDepthFormat(const vk::Format format) {
    for (uint8_t i = 0; i < vk::componentCount(format); ++i)
        if (*vk::componentName(format, i) == 'D') return true;
    return false;
}
}  // namespace

Vulkan::~Vulkan() {
    if (device) {
        device.waitIdle();

        frame.destroy(device, commandPool);
        destroySwapChain();
        vmaDestroyAllocator(vmaAllocator);

        for (auto& renderResource : renderResources) {
            renderResource.renderPassBuilder.destroy(device);
            device.destroyRenderPass(renderResource.renderPass);
        }

        for (auto& drawResource : drawResources) {
            device.destroyPipeline(drawResource.graphicsPipeline);
            device.destroyPipelineLayout(drawResource.pipelineLayout);
            device.destroyDescriptorPool(drawResource.descriptorPool);
            device.destroyDescriptorSetLayout(drawResource.descriptorSetLayout);
        }

        device.freeCommandBuffers(commandPool, commandBuffer);
        device.destroyCommandPool(commandPool);
        device.destroy();
    }
    if (instance) {
        instance.destroySurfaceKHR(surface);
        instance.destroy();
    }
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

void Vulkan::init(vk::Extent2D extent, std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR,
                  uint32_t renderPassCount, std::function<bool(const vk::PhysicalDevice&)> pickDevice) {
    initInstance();
    enumerateDevice(pickDevice);
    initDevice(getSurfaceKHR);
    initSwapChain(extent);
    initCommandBuffer();
    frame.init(device, commandPool);

    renderResources.resize(renderPassCount);
};

void Vulkan::addRenderPass(const RenderPassBuilder& builder) {
    ++renderIndex;
    renderPassBuilder() = builder;

    if (renderPassBuilder().attachmentDescriptions.empty())
        renderPassBuilder() = makeRenderPassBuilder({surfaceFormat.format, vk::Format::eD16Unorm});

    renderPass() = renderPassBuilder().build(device);
    renderPassBuilder().buildDescriptor(device, swapChainImages.size());

    initFrameBuffers();
}

uint32_t Vulkan::attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                              uint32_t vertexStride, const std::vector<vk::Format>& vertexFormats,
                              const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures,
                              uint32_t subpass, vk::CullModeFlags cullMode, bool autoDestroy) {
    drawResources.push_back({});

    if (!uniforms.empty() || !textures.empty()) initDescriptorSet(uniforms, textures);
    uint32_t drawId =
        initPipeline(vertexShaderModule, fragmentShaderModule, vertexStride, vertexFormats, subpass, cullMode);

    if (autoDestroy) {
        destroyShaderModule(fragmentShaderModule);
        destroyShaderModule(vertexShaderModule);
    }

    return drawId;
}

uint32_t Vulkan::attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                              const std::vector<uint32_t>& vertexStrides, const std::vector<vk::Format>& vertexFormats,
                              const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures,
                              vk::PrimitiveTopology primitiveTopology, uint32_t subpass, vk::CullModeFlags cullMode,
                              bool autoDestroy) {
    drawResources.push_back({});

    if (!uniforms.empty() || !textures.empty()) initDescriptorSet(uniforms, textures);
    uint32_t drawId = initPipeline(vertexShaderModule, fragmentShaderModule, vertexStrides, vertexFormats,
                                   primitiveTopology, subpass, cullMode);

    if (autoDestroy) {
        destroyShaderModule(fragmentShaderModule);
        destroyShaderModule(vertexShaderModule);
    }

    return drawId;
}

void Vulkan::renderBegin() {
    device.waitForFences(frame.drawFence(), vk::True, std::numeric_limits<uint64_t>::max());

    auto currentBuffer =
        device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), frame.imageAcquiredSemaphore());
    if (currentBuffer.result == vk::Result::eErrorOutOfDateKHR) {
        throw std::runtime_error("resize");
    } else if (currentBuffer.result != vk::Result::eSuccess && currentBuffer.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("fail to acquire swap chain image!");
    }

    renderIndex = 0;
    assert(currentBuffer.value < framebuffers().size());

    device.resetFences(frame.drawFence());

    std::vector<vk::ClearValue> clearValues(renderPassBuilder().attachmentDescriptions.size(), vk::ClearColorValue{});
    clearValues.front().color = bgColor;
    if (isDepthFormat(renderPassBuilder().attachmentDescriptions.back().format))
        clearValues.back().depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassBeginInfo(renderPass(), framebuffers()[currentBuffer.value],
                                                vk::Rect2D(vk::Offset2D(0, 0), imageExtent), clearValues);

    frame.commandBuffer().begin(vk::CommandBufferBeginInfo());
    frame.commandBuffer().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    frame.commandBuffer().setViewport(0, vk::Viewport(0, 0, (float)imageExtent.width, (float)imageExtent.height, 0, 1));
    frame.commandBuffer().setScissor(0, vk::Rect2D({0, 0}, imageExtent));

    this->currentBuffer = currentBuffer.value;
}

void Vulkan::draw(uint32_t i, const Buffer& vertex) {
    frame.commandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline(i));
    if (descriptorSet(i))
        frame.commandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout(i), 0,
                                                 descriptorSet(i), nullptr);
    if (!renderPassBuilder().descriptorSets.empty())
        frame.commandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout(i), 1,
                                                 renderPassBuilder().descriptorSets[currentBuffer], nullptr);
    frame.commandBuffer().bindVertexBuffers(0, vertex.buffer, {0});
    frame.commandBuffer().draw((uint32_t)vertex.size / vertex.stride, 1, 0, 0);
}

void Vulkan::drawIndexed(uint32_t i, const vk::Buffer& index, vk::DeviceSize indexOffset, vk::IndexType indexType,
                         uint32_t count, const std::vector<vk::Buffer>& vertex,
                         const std::vector<vk::DeviceSize>& vertexOffset) {
    frame.commandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline(i));
    if (descriptorSet(i))
        frame.commandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout(i), 0,
                                                 descriptorSet(i), nullptr);
    if (!renderPassBuilder().descriptorSets.empty())
        frame.commandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout(i), 1,
                                                 renderPassBuilder().descriptorSets[currentBuffer], nullptr);
    frame.commandBuffer().bindVertexBuffers(0, vertex, vertexOffset);
    frame.commandBuffer().bindIndexBuffer(index, indexOffset, indexType);
    frame.commandBuffer().pushConstants<uint32_t>(pipelineLayout(i), vk::ShaderStageFlagBits::eAll, 0, i);
    frame.commandBuffer().drawIndexed(count, 1, 0, 0, 0);
}

void Vulkan::nextSubpass() { frame.commandBuffer().nextSubpass(vk::SubpassContents::eInline); }

void Vulkan::nextPass() {
    ++renderIndex;

    std::vector<vk::ClearValue> clearValues(renderPassBuilder().attachmentDescriptions.size(), vk::ClearColorValue{});
    clearValues.front().color = bgColor;
    if (isDepthFormat(renderPassBuilder().attachmentDescriptions.back().format))
        clearValues.back().depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassBeginInfo(renderPass(), framebuffers()[currentBuffer],
                                                vk::Rect2D(vk::Offset2D(0, 0), imageExtent), clearValues);

    frame.commandBuffer().endRenderPass();
    frame.commandBuffer().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

void Vulkan::renderEnd() {
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

void Vulkan::resize(vk::Extent2D extent) {
    device.waitIdle();
    destroySwapChain();
    initSwapChain(extent);

    for (renderIndex = 0; renderIndex < renderResources.size(); ++renderIndex) initFrameBuffers();
}

Vulkan::Buffer Vulkan::createUniformBuffer(vk::DeviceSize size) {
    Buffer buffer;
    std::tie(buffer.buffer, buffer.memory) =
        createBuffer(vmaAllocator, size, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    buffer.stride = 0;
    buffer.size = size;
    vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);

    return buffer;
}

void Vulkan::destroyUniformBuffer(const Buffer& buffer) {
    if (buffer.size) {
        vmaUnmapMemory(vmaAllocator, buffer.memory);
        vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
    }
}

Vulkan::Buffer Vulkan::createVertexBuffer(const void* vertices, uint32_t stride, size_t size) {
    Buffer buffer;
    buffer.stride = stride;
    buffer.size = stride * size;

    std::tie(buffer.buffer, buffer.memory) =
        createBuffer(vmaAllocator, buffer.size, vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
    memcpy(buffer.data, vertices, buffer.size);
    vmaUnmapMemory(vmaAllocator, buffer.memory);

    return buffer;
}

void Vulkan::destroyVertexBuffer(const Buffer& buffer) {
    if (buffer.size) vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
}

Vulkan::Buffer Vulkan::createGltfBuffer(const void* data, size_t size) {
    Buffer buffer;
    buffer.stride = -1;
    buffer.size = size;

    std::tie(buffer.buffer, buffer.memory) =
        createBuffer(vmaAllocator, buffer.size,
                     vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
                         vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
    memcpy(buffer.data, data, buffer.size);
    vmaUnmapMemory(vmaAllocator, buffer.memory);

    return buffer;
}
void Vulkan::destroyGltfBuffer(const Buffer& buffer) {
    if (buffer.size) vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.memory);
}

Vulkan::Texture Vulkan::createTexture(vk::Extent2D extent, const void* data, uint32_t layers, bool cubemap,
                                      bool anisotropy, vk::Filter mag, vk::Filter min, vk::SamplerAddressMode modeU,
                                      vk::SamplerAddressMode modeV, vk::SamplerAddressMode modeW) {
    Texture texture;
    texture.format = vk::Format::eR8G8B8A8Unorm;

    vk::FormatProperties formatProps = physicalDevice.getFormatProperties(texture.format);
    uint32_t mipLevels = (uint32_t)std::log2(std::max(extent.width, extent.height)) + 1;

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
        vmaAllocator, extent, texture.format, needStaging ? vk::ImageTiling::eOptimal : vk::ImageTiling::eLinear,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
        needStaging ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePreinitialized, mipLevels, layers, cubemap);

    if (needStaging)
        std::tie(texture.stagingBuffer, texture.stagingMemory) =
            createBuffer(vmaAllocator, extent.width * extent.height * layers * 4, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* textureData;
    vmaMapMemory(vmaAllocator, needStaging ? texture.stagingMemory : texture.memory, &textureData);
    memcpy(textureData, data, extent.width * extent.height * layers * 4);
    vmaUnmapMemory(vmaAllocator, needStaging ? texture.stagingMemory : texture.memory);

    commandBuffer.begin(vk::CommandBufferBeginInfo());
    if (needStaging) {
        setImageLayout(commandBuffer, texture.image, texture.format, vk::ImageLayout::eUndefined,
                       vk::ImageLayout::eTransferDstOptimal, 0, layers);
        vk::BufferImageCopy copyRegion(0, extent.width, extent.height, {vk::ImageAspectFlagBits::eColor, 0, 0, layers},
                                       vk::Offset3D(0, 0, 0), vk::Extent3D(extent, 1));
        commandBuffer.copyBufferToImage(texture.stagingBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal,
                                        copyRegion);
        setImageLayout(commandBuffer, texture.image, texture.format, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal, 0, layers);
    } else {
        setImageLayout(commandBuffer, texture.image, texture.format, vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferSrcOptimal, 0, layers);
    }
    for (uint32_t i = 1; i < mipLevels; ++i) {
        vk::ImageBlit imageBlit({vk::ImageAspectFlagBits::eColor, i - 1, 0, layers},
                                {{{}, {int32_t(extent.width >> (i - 1)), int32_t(extent.height >> (i - 1)), 1}}},
                                {vk::ImageAspectFlagBits::eColor, i, 0, layers},
                                {{{}, {int32_t(extent.width >> i), int32_t(extent.height >> i), 1}}});
        setImageLayout(commandBuffer, texture.image, texture.format,
                       needStaging ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePreinitialized,
                       vk::ImageLayout::eTransferDstOptimal, i, layers);
        commandBuffer.blitImage(texture.image, vk::ImageLayout::eTransferSrcOptimal, texture.image,
                                vk::ImageLayout::eTransferDstOptimal, imageBlit, vk::Filter::eLinear);
        setImageLayout(commandBuffer, texture.image, texture.format, vk::ImageLayout::eTransferDstOptimal,
                       vk::ImageLayout::eTransferSrcOptimal, i, layers);
    }
    setImageLayout(commandBuffer, texture.image, texture.format, vk::ImageLayout::eTransferSrcOptimal,
                   vk::ImageLayout::eShaderReadOnlyOptimal, 0, layers, mipLevels);
    commandBuffer.end();

    vk::Fence fence = device.createFence({});
    graphicsQueue.submit(vk::SubmitInfo({}, {}, commandBuffer), fence);
    device.waitForFences(fence, vk::True, std::numeric_limits<uint64_t>::max());
    device.destroyFence(fence);

    vk::SamplerCreateInfo samplerCreateInfo({}, mag, min, vk::SamplerMipmapMode::eLinear, modeU, modeV, modeW, 0.0f,
                                            anisotropy, physicalDevice.getProperties().limits.maxSamplerAnisotropy,
                                            false, vk::CompareOp::eNever, 0.0f, (float)mipLevels,
                                            vk::BorderColor::eFloatOpaqueBlack);
    texture.sampler = device.createSampler(samplerCreateInfo);

    vk::ImageViewCreateInfo imageViewCreateInfo(
        {}, texture.image, layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray, texture.format, {},
        {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, layers});
    if (cubemap) imageViewCreateInfo.setViewType(vk::ImageViewType::eCube);
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

    if (!applicationInfo.apiVersion) applicationInfo.apiVersion = VK_API_VERSION_1_1;
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
    graphicsQueueFamliyIndex = (uint32_t)std::distance(queueFamilyProperties.begin(), it);
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
    deviceFeatures.imageCubeArray = vk::True;
    deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);

    device = physicalDevice.createDevice(deviceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    graphicsQueue = device.getQueue(graphicsQueueFamliyIndex, 0);
    presentationQueue = device.getQueue(presentationQueueFamliyIndex, 0);
    computeQueue = graphicsQueue;

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
    surfaceFormat = pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));

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

    minImageCount = clamp(3u, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        {}, surface, minImageCount, surfaceFormat.format, surfaceFormat.colorSpace, imageExtent, 1,
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

void Vulkan::initDescriptorSet(const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures) {
    assert(!uniforms.empty() || !textures.empty());

    std::array<vk::DescriptorPoolSize, 2> poolSizes;
    poolSizes[0] = {vk::DescriptorType::eUniformBuffer, (uint32_t)uniforms.size()};
    poolSizes[1] = {vk::DescriptorType::eCombinedImageSampler, (uint32_t)textures.size()};
    descriptorPool() = device.createDescriptorPool({{},
                                                    1,
                                                    poolSizes[0].descriptorCount == 0   ? 1u
                                                    : poolSizes[1].descriptorCount == 0 ? 1u
                                                                                        : 2u,
                                                    poolSizes[0].descriptorCount == 0   ? &poolSizes[1]
                                                    : poolSizes[1].descriptorCount == 0 ? &poolSizes[0]
                                                                                        : poolSizes.data()});

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    descriptorSetLayoutBindings.reserve(uniforms.size() + textures.size());
    for (auto uniform : uniforms)
        descriptorSetLayoutBindings.emplace_back(uniform.first, vk::DescriptorType::eUniformBuffer, 1,
                                                 uniform.second.stage);
    for (auto texture : textures)
        descriptorSetLayoutBindings.emplace_back(texture.first, vk::DescriptorType::eCombinedImageSampler, 1,
                                                 texture.second.stage);
    descriptorSetLayout() = device.createDescriptorSetLayout({{}, descriptorSetLayoutBindings});

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool(), 1, &descriptorSetLayout());
    device.allocateDescriptorSets(&descriptorSetAllocateInfo, &descriptorSet());

    std::vector<vk::DescriptorBufferInfo> descriptorBufferInfo;
    descriptorBufferInfo.reserve(uniforms.size());
    std::vector<vk::DescriptorImageInfo> descriptorImageInfo;
    descriptorImageInfo.reserve(textures.size());
    std::vector<vk::WriteDescriptorSet> writeDescriptorSet;
    writeDescriptorSet.reserve(uniforms.size() + textures.size());

    for (const auto& uniform : uniforms) {
        descriptorBufferInfo.emplace_back(uniform.second.buffer, 0, uniform.second.size);
        writeDescriptorSet.emplace_back(descriptorSet(), uniform.first, 0, 1, vk::DescriptorType::eUniformBuffer,
                                        nullptr, &descriptorBufferInfo.back());
    }
    for (const auto& texture : textures) {
        descriptorImageInfo.emplace_back(texture.second.sampler, texture.second.view,
                                         isDepthFormat(texture.second.format)
                                             ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                             : vk::ImageLayout::eShaderReadOnlyOptimal);
        writeDescriptorSet.emplace_back(descriptorSet(), texture.first, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                        &descriptorImageInfo.back());
    }

    device.updateDescriptorSets(writeDescriptorSet, nullptr);
}

void Vulkan::initFrameBuffers() {
    renderPassBuilder().buildImages(*this);
    renderPassBuilder().buildDescriptorSets(device, swapChainImages.size());

    std::vector<vk::ImageView> attachments(renderPassBuilder().attachmentDescriptions.size());
    vk::FramebufferCreateInfo framebufferCreateInfo({}, renderPass(), attachments, imageExtent.width,
                                                    imageExtent.height, 1);

    framebuffers().reserve(swapChainImageViews.size());
    for (unsigned j = 0; j < swapChainImageViews.size(); ++j) {
        if (renderPassBuilder().offscreenColor) {
            unsigned i = 0;
            for (auto offscreenColorTexture : renderPassBuilder().offscreenColorTextures)
                attachments[i++] = offscreenColorTexture->view;
            if (isDepthFormat(renderPassBuilder().attachmentDescriptions.back().format))
                attachments.back() = renderPassBuilder().imageViews[j].back();
        } else {
            attachments[0] = swapChainImageViews[j];
            for (unsigned i = 0; i < renderPassBuilder().attachmentDescriptions.size() - 1; ++i)
                attachments[1 + i] = renderPassBuilder().imageViews[j][i];
        }
        if (renderPassBuilder().offscreenDepth) attachments.back() = renderPassBuilder().offscreenDepthTexture->view;
        framebuffers().push_back(device.createFramebuffer(framebufferCreateInfo));
    }
}

uint32_t Vulkan::initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                              uint32_t vertexStride, const std::vector<vk::Format>& vertexFormats, uint32_t subpass,
                              vk::CullModeFlags cullMode) {
    vk::VertexInputBindingDescription vertexInputBindingDescription(0, vertexStride);

    std::vector<vk::VertexInputAttributeDescription> vertexInputAtrributeDescriptions;
    vertexInputAtrributeDescriptions.reserve(vertexFormats.size());

    uint32_t offset = 0;
    for (uint32_t i = 0; i < vertexFormats.size(); ++i) {
        vertexInputAtrributeDescriptions.emplace_back(i, 0, vertexFormats[i], offset);
        offset += vk::blockSize(vertexFormats[i]);
    }

    return initPipeline(vertexShaderModule, fragmentShaderModule,
                        {{}, vertexInputBindingDescription, vertexInputAtrributeDescriptions},
                        vk::PrimitiveTopology::eTriangleList, subpass, cullMode);
}

uint32_t Vulkan::initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                              const std::vector<uint32_t>& vertexStrides, const std::vector<vk::Format>& vertexFormats,
                              vk::PrimitiveTopology primitiveTopology, uint32_t subpass, vk::CullModeFlags cullMode) {
    std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions;
    vertexInputBindingDescriptions.reserve(vertexStrides.size());
    for (int i = 0; i < vertexStrides.size(); ++i) vertexInputBindingDescriptions.emplace_back(i, vertexStrides[i]);

    std::vector<vk::VertexInputAttributeDescription> vertexInputAtrributeDescriptions;
    vertexInputAtrributeDescriptions.reserve(vertexFormats.size());
    for (uint32_t i = 0; i < vertexFormats.size(); ++i)
        vertexInputAtrributeDescriptions.emplace_back(i, i, vertexFormats[i], 0);

    assert(4 <= physicalDevice.getProperties().limits.maxPushConstantsSize);
    return initPipeline(vertexShaderModule, fragmentShaderModule,
                        {{}, vertexInputBindingDescriptions, vertexInputAtrributeDescriptions}, primitiveTopology,
                        subpass, cullMode, {vk::ShaderStageFlagBits::eAll, 0, 4});
}

uint32_t Vulkan::initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                              const vk::PipelineVertexInputStateCreateInfo& vertexInfo,
                              vk::PrimitiveTopology primitiveTopology, uint32_t subpass, vk::CullModeFlags cullMode,
                              const vk::PushConstantRange& pushConstant) {
    std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShaderModule, "main")};

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo({}, primitiveTopology);

    vk::Viewport viewport(0, 0, (float)imageExtent.width, (float)imageExtent.height, 0, 1);
    vk::Rect2D scissor({0, 0}, imageExtent);
    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo({}, viewport, scissor);

    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
        {}, false, false, vk::PolygonMode::eFill, cullMode, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f,
        1.0f);
    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

    bool depthBuffered = renderPassBuilder().depthReference != nullptr;
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo({}, depthBuffered, depthBuffered,
                                                                                vk::CompareOp::eLessOrEqual);

    std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates(
        renderPassBuilder().subpassDescriptions[subpass].colorAttachmentCount,
        {true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne,
         vk::BlendFactor::eZero, vk::BlendOp::eAdd,
         vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
             vk::ColorComponentFlagBits::eA});
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eNoOp,
                                                                            pipelineColorBlendAttachmentStates);

    std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo({}, dynamicStates);

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.reserve(2);

    if (descriptorSetLayout()) descriptorSetLayouts.push_back(descriptorSetLayout());
    if (renderPassBuilder().descriptorSetLayout)
        descriptorSetLayouts.push_back(renderPassBuilder().descriptorSetLayout);

    pipelineLayout() = device.createPipelineLayout({{},
                                                    (uint32_t)descriptorSetLayouts.size(),
                                                    descriptorSetLayouts.data(),
                                                    pushConstant.size ? 1u : 0,
                                                    pushConstant.size ? &pushConstant : nullptr});

    vk::GraphicsPipelineCreateInfo graphicPipelineCreateInfo(
        {}, pipelineShaderStageCreateInfos, &vertexInfo, &pipelineInputAssemblyStateCreateInfo, nullptr,
        &pipelineViewportStateCreateInfo, &pipelineRasterizationStateCreateInfo, &pipelineMultisampleStateCreateInfo,
        &pipelineDepthStencilStateCreateInfo, &pipelineColorBlendStateCreateInfo, &pipelineDynamicStateCreateInfo,
        pipelineLayout(), renderPass(), subpass);

    vk::Result result;
    std::tie(result, graphicsPipeline()) = device.createGraphicsPipeline(nullptr, graphicPipelineCreateInfo);
    assert(result == vk::Result::eSuccess);

    return (uint32_t)drawResources.size() - 1;
}

void Vulkan::destroySwapChain() {
    for (auto& renderResource : renderResources) {
        for (auto const& framebuffer : renderResource.framebuffers) device.destroyFramebuffer(framebuffer);
        renderResource.framebuffers.clear();
        renderResource.renderPassBuilder.destroyImages(device, vmaAllocator);
    }

    for (auto& swapChainImageView : swapChainImageViews) device.destroyImageView(swapChainImageView);
    swapChainImageViews.clear();
    device.destroySwapchainKHR(swapChain);
}

Vulkan::RenderPassBuilder& Vulkan::RenderPassBuilder::addSubpass(const std::initializer_list<uint32_t>& colors,
                                                                 const std::initializer_list<uint32_t>& inputs) {
    auto colorReferences = std::make_shared<std::vector<vk::AttachmentReference>>();
    colorReferences->reserve(colors.size());
    for (auto color : colors) colorReferences->emplace_back(color, vk::ImageLayout::eColorAttachmentOptimal);

    auto inputReferences = std::make_shared<std::vector<vk::AttachmentReference>>();
    if (inputs.size()) inputReferences->reserve(inputs.size());
    for (auto input : inputs) inputReferences->emplace_back(input, vk::ImageLayout::eShaderReadOnlyOptimal);

    subpassDescriptions.emplace_back(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, *inputReferences,
                                     *colorReferences, vk::ArrayProxyNoTemporaries<const vk::AttachmentReference>{},
                                     depthReference.get());

    attachmentReferences.push_back(colorReferences);
    attachmentReferences.push_back(inputReferences);

    return *this;
}

Vulkan::RenderPassBuilder& Vulkan::RenderPassBuilder::dependOn(uint32_t subpass) {
    dependencies.emplace_back(subpass, subpassDescriptions.size() - 1,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::AccessFlagBits::eInputAttachmentRead, vk::DependencyFlagBits::eByRegion);
    return *this;
}

void Vulkan::RenderPassBuilder::buildImages(Vulkan& vulkan) {
    images.reserve(vulkan.swapChainImages.size());
    imageViews.reserve(vulkan.swapChainImages.size());

    for (unsigned j = 0; j < vulkan.swapChainImages.size(); ++j) {
        images.push_back({});
        imageViews.push_back({});

        images.back().reserve(attachmentDescriptions.size() - 1);
        imageViews.back().reserve(attachmentDescriptions.size() - 1);

        auto colorUsage =
            vk::ImageUsageFlagBits::eColorAttachment |
            (offscreenColor ? vk::ImageUsageFlagBits::eSampled
                            : vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eTransientAttachment);

        for (unsigned i = 1; i < attachmentDescriptions.size() - 1; ++i) {
            vk::FormatProperties formatProp =
                vulkan.physicalDevice.getFormatProperties(attachmentDescriptions[i].format);

            vk::ImageTiling tiling;
            if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
                tiling = vk::ImageTiling::eOptimal;
            } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
                tiling = vk::ImageTiling::eLinear;
            } else {
                throw std::runtime_error("ColorAttachment format not supported.");
            }

            auto image = createImage(vulkan.vmaAllocator, vulkan.imageExtent, attachmentDescriptions[i].format, tiling,
                                     colorUsage);
            vk::ImageView imageView = vulkan.device.createImageView({{},
                                                                     image.first,
                                                                     vk::ImageViewType::e2D,
                                                                     attachmentDescriptions[i].format,
                                                                     {},
                                                                     {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
            images.back().push_back(image);
            imageViews.back().push_back(imageView);
        }

        if (isDepthFormat(attachmentDescriptions.back().format)) {
            vk::FormatProperties formatProp =
                vulkan.physicalDevice.getFormatProperties(attachmentDescriptions.back().format);

            vk::ImageTiling tiling;
            if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                tiling = vk::ImageTiling::eOptimal;
            } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                tiling = vk::ImageTiling::eLinear;
            } else {
                throw std::runtime_error("DepthAttachment format not supported.");
            }

            auto image =
                createImage(vulkan.vmaAllocator, vulkan.imageExtent, attachmentDescriptions.back().format, tiling,
                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment |
                                vk::ImageUsageFlagBits::eTransientAttachment);
            vk::ImageView imageView = vulkan.device.createImageView({{},
                                                                     image.first,
                                                                     vk::ImageViewType::e2D,
                                                                     attachmentDescriptions.back().format,
                                                                     {},
                                                                     {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});
            images.back().push_back(image);
            imageViews.back().push_back(imageView);
        } else if (attachmentDescriptions.size() > 1) {
            vk::FormatProperties formatProp =
                vulkan.physicalDevice.getFormatProperties(attachmentDescriptions.back().format);

            vk::ImageTiling tiling;
            if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
                tiling = vk::ImageTiling::eOptimal;
            } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
                tiling = vk::ImageTiling::eLinear;
            } else {
                throw std::runtime_error("ColorAttachment format not supported.");
            }

            auto image = createImage(vulkan.vmaAllocator, vulkan.imageExtent, attachmentDescriptions.back().format,
                                     tiling, colorUsage);
            vk::ImageView imageView = vulkan.device.createImageView({{},
                                                                     image.first,
                                                                     vk::ImageViewType::e2D,
                                                                     attachmentDescriptions.back().format,
                                                                     {},
                                                                     {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
            images.back().push_back(image);
            imageViews.back().push_back(imageView);
        }
    }

    if (offscreenColor) {
        vk::FormatProperties formatProp =
            vulkan.physicalDevice.getFormatProperties(attachmentDescriptions.front().format);

        vk::ImageTiling tiling;
        if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
            tiling = vk::ImageTiling::eOptimal;
        } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) {
            tiling = vk::ImageTiling::eLinear;
        } else {
            throw std::runtime_error("ColorAttachment format not supported.");
        }

        auto offscreenColorTexture = std::make_shared<Texture>();
        offscreenColorTexture->format = attachmentDescriptions.front().format;
        std::tie(offscreenColorTexture->image, offscreenColorTexture->memory) =
            createImage(vulkan.vmaAllocator, vulkan.imageExtent, offscreenColorTexture->format, tiling,
                        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
        offscreenColorTexture->view = vulkan.device.createImageView({{},
                                                                     offscreenColorTexture->image,
                                                                     vk::ImageViewType::e2D,
                                                                     offscreenColorTexture->format,
                                                                     {},
                                                                     {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
        offscreenColorTexture->sampler = vulkan.device.createSampler({});

        auto textureCount =
            attachmentDescriptions.size() - (isDepthFormat(attachmentDescriptions.back().format) ? 1 : 0);
        offscreenColorTextures.reserve(textureCount);

        offscreenColorTextures.push_back(offscreenColorTexture);

        for (unsigned i = 1; i < textureCount; ++i) {
            auto offscreenColorTexture = std::make_shared<Texture>();
            offscreenColorTexture->format = attachmentDescriptions[i].format;
            std::tie(offscreenColorTexture->image, offscreenColorTexture->memory) = images.front()[i - 1];
            offscreenColorTexture->view = imageViews.front()[i - 1];
            offscreenColorTexture->sampler = vulkan.device.createSampler({});

            offscreenColorTextures.push_back(offscreenColorTexture);
        }
    }
    if (offscreenDepth) {
        vk::FormatProperties formatProp =
            vulkan.physicalDevice.getFormatProperties(attachmentDescriptions.back().format);

        vk::ImageTiling tiling;
        if (formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eOptimal;
        } else if (formatProp.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eLinear;
        } else {
            throw std::runtime_error("DepthAttachment format not supported.");
        }

        offscreenDepthTexture = std::make_shared<Texture>();
        offscreenDepthTexture->format = attachmentDescriptions.back().format;
        std::tie(offscreenDepthTexture->image, offscreenDepthTexture->memory) =
            createImage(vulkan.vmaAllocator, vulkan.imageExtent, offscreenDepthTexture->format, tiling,
                        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
        offscreenDepthTexture->view = vulkan.device.createImageView({{},
                                                                     offscreenDepthTexture->image,
                                                                     vk::ImageViewType::e2D,
                                                                     offscreenDepthTexture->format,
                                                                     {},
                                                                     {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});
        offscreenDepthTexture->sampler = vulkan.device.createSampler({});
    }
}

void Vulkan::RenderPassBuilder::buildDescriptor(const vk::Device& device, size_t swapChainImageCount) {
    // we create input descriptor pool for every attachment except the framebuffer
    uint32_t descriptorCount = swapChainImageCount * (attachmentDescriptions.size() - 1);
    if (!descriptorCount) return;

    vk::DescriptorPoolSize poolSize{vk::DescriptorType::eInputAttachment, descriptorCount};

    descriptorPool = device.createDescriptorPool({{}, (uint32_t)swapChainImageCount, 1, &poolSize});

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    descriptorSetLayoutBindings.reserve(attachmentDescriptions.size() - 1);

    for (unsigned i = 0; i < attachmentDescriptions.size() - 1; ++i)
        descriptorSetLayoutBindings.emplace_back(i, vk::DescriptorType::eInputAttachment, 1,
                                                 vk::ShaderStageFlagBits::eFragment);
    descriptorSetLayout = device.createDescriptorSetLayout({{}, descriptorSetLayoutBindings});

    std::vector<vk::DescriptorSetLayout> setLayouts(swapChainImageCount, descriptorSetLayout);

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, setLayouts);
    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);
}

void Vulkan::RenderPassBuilder::buildDescriptorSets(const vk::Device& device, size_t swapChainImageCount) {
    uint32_t descriptorCount = swapChainImageCount * (attachmentDescriptions.size() - 1);
    if (!descriptorCount) return;

    std::vector<vk::DescriptorImageInfo> descriptorImageInfo;
    descriptorImageInfo.reserve(descriptorCount);
    std::vector<vk::WriteDescriptorSet> writeDescriptorSet;
    writeDescriptorSet.reserve(descriptorCount);

    for (unsigned j = 0; j < swapChainImageCount; ++j)
        for (unsigned i = 0; i < attachmentDescriptions.size() - 1; ++i) {
            descriptorImageInfo.emplace_back(nullptr, imageViews[j][i], vk::ImageLayout::eShaderReadOnlyOptimal);
            writeDescriptorSet.emplace_back(descriptorSets[j], i, 0, 1, vk::DescriptorType::eInputAttachment,
                                            &descriptorImageInfo.back());
        }

    device.updateDescriptorSets(writeDescriptorSet, nullptr);
}

void Vulkan::RenderPassBuilder::destroy(const vk::Device& device) {
    if (descriptorPool) device.destroyDescriptorPool(descriptorPool);
    if (descriptorSetLayout) device.destroyDescriptorSetLayout(descriptorSetLayout);
}

void Vulkan::RenderPassBuilder::destroyImages(const vk::Device& device, const VmaAllocator& vmaAllocator) {
    for (auto& imageView : imageViews)
        for (auto& view : imageView) device.destroyImageView(view);
    imageViews.clear();

    for (auto& image : images)
        for (auto& img : image) vmaDestroyImage(vmaAllocator, img.first, img.second);
    images.clear();

    if (offscreenColor) {
        device.destroySampler(offscreenColorTextures.front()->sampler);
        device.destroyImageView(offscreenColorTextures.front()->view);
        vmaDestroyImage(vmaAllocator, offscreenColorTextures.front()->image, offscreenColorTextures.front()->memory);
    }
    if (offscreenDepth) {
        device.destroySampler(offscreenDepthTexture->sampler);
        device.destroyImageView(offscreenDepthTexture->view);
        vmaDestroyImage(vmaAllocator, offscreenDepthTexture->image, offscreenDepthTexture->memory);
    }
}

vk::RenderPass Vulkan::RenderPassBuilder::build(const vk::Device& device) {
    if (subpassDescriptions.empty()) {
        if (isDepthFormat(attachmentDescriptions.front().format))
            addSubpass();
        else
            addSubpass({0});
    }

    // This makes sure that writes to the depth image are done before we try to write to it again
    dependencies.emplace_back(
        vk::SubpassExternal, 0,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::AccessFlagBits::eNone, vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    dependencies.emplace_back(vk::SubpassExternal, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eNone,
                              vk::AccessFlagBits::eColorAttachmentWrite);
    if (offscreenColor) {
        dependencies.emplace_back(vk::SubpassExternal, 0, vk::PipelineStageFlagBits::eBottomOfPipe,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eMemoryRead,
                                  vk::AccessFlagBits::eColorAttachmentWrite, vk::DependencyFlagBits::eByRegion);
    }
    if (offscreenDepth) {
        dependencies.emplace_back(vk::SubpassExternal, 0, vk::PipelineStageFlagBits::eFragmentShader,
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eShaderRead,
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::DependencyFlagBits::eByRegion);
        dependencies.emplace_back(
            subpassDescriptions.size() - 1, vk::SubpassExternal, vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eShaderRead, vk::DependencyFlagBits::eByRegion);
    }
    dependencies.emplace_back(subpassDescriptions.size() - 1, vk::SubpassExternal,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eBottomOfPipe,
                              vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                              vk::AccessFlagBits::eMemoryRead, vk::DependencyFlagBits::eByRegion);

    return device.createRenderPass({{}, attachmentDescriptions, subpassDescriptions, dependencies});
}

Vulkan::RenderPassBuilder Vulkan::makeRenderPassBuilder(const vk::ArrayProxy<vk::Format>& formats, bool preserveContent,
                                                        bool offscreenColor, bool offscreenDepth) {
    RenderPassBuilder builder;
    builder.offscreenColor = offscreenColor;
    builder.offscreenDepth = offscreenDepth;

    // The first attachment is always the framebuffer image
    // and the last attachment is always the depth image (if suitable)

    auto format = formats.begin();

    builder.attachmentDescriptions.emplace_back(
        vk::AttachmentDescriptionFlags{}, *format++, vk::SampleCountFlagBits::e1,
        preserveContent ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        preserveContent ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eUndefined,
        offscreenColor ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::ePresentSrcKHR);

    while (format != formats.end())
        builder.attachmentDescriptions.emplace_back(
            vk::AttachmentDescriptionFlags{}, *format++, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
            offscreenColor ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
            offscreenColor ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eColorAttachmentOptimal);

    if (isDepthFormat(formats.back())) {
        auto finalLayout = offscreenDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                          : vk::ImageLayout::eDepthStencilAttachmentOptimal;
        builder.attachmentDescriptions.back().setInitialLayout(vk::ImageLayout::eUndefined);
        builder.attachmentDescriptions.back().setFinalLayout(finalLayout);
        if (offscreenDepth) builder.attachmentDescriptions.back().setStoreOp(vk::AttachmentStoreOp::eStore);
        builder.depthReference = std::make_shared<vk::AttachmentReference>(
            static_cast<uint32_t>(builder.attachmentDescriptions.size() - 1), finalLayout);
        return builder;
    }

    builder.depthReference = nullptr;
    return builder;
}

Vulkan::RenderPassBuilder Vulkan::makeRenderPassBuilder(const vk::ArrayProxy<vk::AttachmentDescription>& attachments,
                                                        uint32_t depth) {
    RenderPassBuilder builder;
    builder.offscreenDepth = false;
    builder.offscreenColor = false;

    for (auto& attachment : attachments) builder.attachmentDescriptions.push_back(attachment);

    builder.depthReference =
        depth != -1 ? std::make_shared<vk::AttachmentReference>(depth, vk::ImageLayout::eDepthStencilAttachmentOptimal)
                    : nullptr;

    return builder;
}
