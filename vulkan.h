#pragma once

#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <functional>

class Vulkan {
   public:
    friend class Engine;

    struct Buffer {
        vk::Buffer buffer = {};
        VmaAllocation memory = {};
        void* data = nullptr;

        size_t size = 0;
        uint32_t stride = 0;
        vk::ShaderStageFlags stage = vk::ShaderStageFlagBits::eVertex;
    };

    struct Texture {
        vk::Image image = {};
        vk::ImageView view = {};
        VmaAllocation memory = {};
        vk::Sampler sampler = {};

        vk::Buffer stagingBuffer = {};
        VmaAllocation stagingMemory = {};
        vk::ShaderStageFlags stage = vk::ShaderStageFlagBits::eFragment;
    };

    struct RenderPassBuilder {
        RenderPassBuilder& addSubpass(const std::initializer_list<uint32_t>& colors,
                                      const std::initializer_list<uint32_t>& inputs = {});
        RenderPassBuilder& dependOn(uint32_t subpass);

        void buildImages(Vulkan& vulkan);
        void buildDescriptor(const vk::Device& device, size_t swapChainImageCount);
        void buildDescriptorSets(const vk::Device& device, size_t swapChainImageCount);

        vk::RenderPass build(const vk::Device& device, const vk::Format& frameFormat);

        void destroy(const vk::Device& device);
        void destroyImages(const vk::Device& device, const VmaAllocator& vmaAllocator);

        std::shared_ptr<vk::AttachmentReference> depthReference;
        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        std::vector<vk::SubpassDescription> subpassDescriptions;
        std::vector<vk::SubpassDependency> dependencies;

        std::vector<std::vector<std::pair<vk::Image, VmaAllocation>>> images;
        std::vector<std::vector<vk::ImageView>> imageViews;

        vk::DescriptorSetLayout descriptorSetLayout;
        vk::DescriptorPool descriptorPool;
        std::vector<vk::DescriptorSet> descriptorSets;

        std::vector<std::shared_ptr<std::vector<vk::AttachmentReference>>> attachmentReferences;
    };

    Vulkan() = default;
    ~Vulkan();

    Vulkan& setAppInfo(const char* appName, uint32_t appVerson = {});
    Vulkan& setEngineInfo(const char* engineName, uint32_t engineVersion = {});
    Vulkan& setApiVersion(uint32_t apiVersion);
    Vulkan& setInstanceLayers(const vk::ArrayProxyNoTemporaries<const char* const>& layers);
    Vulkan& setInstanceExtensions(const vk::ArrayProxyNoTemporaries<const char* const>& extensions);
    Vulkan& setDeviceLayers(const vk::ArrayProxyNoTemporaries<const char* const>& layers);
    Vulkan& setDeviceExtensions(const vk::ArrayProxyNoTemporaries<const char* const>& extensions);
    Vulkan& setDeviceFeatures(const vk::PhysicalDeviceFeatures& features);
    Vulkan& setRenderPassBuilder(const RenderPassBuilder& builder);

    void init(vk::Extent2D extent, std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR,
              std::function<bool(const vk::PhysicalDevice&)> pickDevice = {});
    uint32_t attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                          const Buffer& vertex, const std::vector<vk::Format>& vertexFormats,
                          const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures,
                          uint32_t subpass, vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
                          bool autoDestroy = true);
    uint32_t attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                          const std::vector<uint32_t>& vertexStrides, const std::vector<vk::Format>& vertexFormats,
                          const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures,
                          vk::PrimitiveTopology primitiveTopology, uint32_t subpass,
                          vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack, bool autoDestroy = true);
    uint32_t renderBegin();
    void updateVertex(uint32_t i, const Buffer& vertex);
    void draw(uint32_t currentBuffer, uint32_t i);
    void drawIndex(uint32_t currentBuffer, uint32_t i, const vk::Buffer& index, vk::DeviceSize indexOffset,
                   vk::IndexType indexType, uint32_t count, const std::vector<vk::Buffer>& vertex,
                   const std::vector<vk::DeviceSize>& vertexOffset);
    void nextSubpass();
    void renderEnd(uint32_t currentBuffer);
    void render();
    void resize(vk::Extent2D extent);

    Buffer createUniformBuffer(vk::DeviceSize size);
    void destroyUniformBuffer(const Buffer& buffer);

    Buffer createVertexBuffer(const void* vertices, uint32_t stride, size_t size);
    void destroyVertexBuffer(const Buffer& buffer);

    Buffer createGltfBuffer(const void* data, size_t size);
    void destroyGltfBuffer(const Buffer& buffer);

    Texture createTexture(vk::Extent2D extent, const void* data, uint32_t layers = 1, bool anisotropy = true,
                          vk::Filter mag = vk::Filter::eLinear, vk::Filter min = vk::Filter::eLinear,
                          vk::SamplerAddressMode modeU = vk::SamplerAddressMode::eRepeat,
                          vk::SamplerAddressMode modeV = vk::SamplerAddressMode::eRepeat,
                          vk::SamplerAddressMode modeW = vk::SamplerAddressMode::eRepeat);
    void destroyTexture(const Texture& texture);

    vk::ShaderModule createShaderModule(vk::ShaderStageFlagBits shaderStage, const std::string& shaderText);
    void destroyShaderModule(const vk::ShaderModule& shader);

    static RenderPassBuilder makeRenderPassBuilder(const vk::ArrayProxy<vk::Format>& formats);
    static RenderPassBuilder makeRenderPassBuilder(const vk::ArrayProxy<vk::AttachmentDescription>& attachments = {});

   private:
    void initInstance();
    void enumerateDevice(std::function<bool(const vk::PhysicalDevice&)> pickDevice);
    void initDevice(std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR);
    void initCommandBuffer();
    void initSwapChain(vk::Extent2D extent);
    void initRenderPass();
    void initFrameBuffers();
    void initDescriptorSet(const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures);
    uint32_t initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                          uint32_t vertexStride, const std::vector<vk::Format>& vertexFormats, uint32_t subpass,
                          vk::CullModeFlags cullMode, bool depthBuffered = true);
    uint32_t initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                          const std::vector<uint32_t>& vertexStrides, const std::vector<vk::Format>& vertexFormats,
                          vk::PrimitiveTopology primitiveTopology, uint32_t subpass, vk::CullModeFlags cullMode,
                          bool depthBuffered = true);
    uint32_t initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                          const vk::PipelineVertexInputStateCreateInfo& vertexInfo,
                          vk::PrimitiveTopology primitiveTopology, uint32_t subpass, vk::CullModeFlags cullMode,
                          bool depthBuffered, const vk::PushConstantRange& pushConstant = {});
    void destroySwapChain();

    //
    // Utils
    //

    vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    std::pair<vk::Buffer, VmaAllocation> createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage,
                                                      vk::MemoryPropertyFlags bufferProp = {});
    std::pair<vk::Image, VmaAllocation> createImage(vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling,
                                                    vk::ImageUsageFlags usage,
                                                    vk::ImageLayout layout = vk::ImageLayout::eUndefined,
                                                    uint32_t mipLevels = 1, uint32_t layers = 1);
    void setImageLayout(const vk::CommandBuffer& commandBuffer, vk::Image image, vk::Format format,
                        vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel = 0,
                        uint32_t layerCount = 1, uint32_t mipCount = 1);

    vk::ApplicationInfo applicationInfo = {};
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
    vk::Queue computeQueue;
    VmaAllocator vmaAllocator;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::Extent2D imageExtent;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> framebuffers;
    vk::RenderPass renderPass;
    vk::ClearColorValue bgColor;
    uint32_t imageCount;

    RenderPassBuilder renderPassBuilder;

    struct DrawResource {
        Buffer vertexBuffer = {};
        vk::DescriptorSetLayout descriptorSetLayout = {};
        vk::DescriptorPool descriptorPool = {};
        vk::DescriptorSet descriptorSet = {};
        vk::PipelineLayout pipelineLayout = {};
        vk::Pipeline graphicsPipeline = {};
    };
    std::vector<DrawResource> drawResources;

    Buffer& vertexBuffer(size_t i = -1) { return drawResources[i == -1 ? drawResources.size() - 1 : i].vertexBuffer; }
    vk::DescriptorSetLayout& descriptorSetLayout(size_t i = -1) {
        return drawResources[i == -1 ? drawResources.size() - 1 : i].descriptorSetLayout;
    }
    vk::DescriptorPool& descriptorPool(size_t i = -1) {
        return drawResources[i == -1 ? drawResources.size() - 1 : i].descriptorPool;
    }
    vk::DescriptorSet& descriptorSet(size_t i = -1) {
        return drawResources[i == -1 ? drawResources.size() - 1 : i].descriptorSet;
    }
    vk::PipelineLayout& pipelineLayout(size_t i = -1) {
        return drawResources[i == -1 ? drawResources.size() - 1 : i].pipelineLayout;
    }
    vk::Pipeline& graphicsPipeline(size_t i = -1) {
        return drawResources[i == -1 ? drawResources.size() - 1 : i].graphicsPipeline;
    }

    struct FrameInFlight {
        static constexpr int FRAME_IN_FLIGHT = 3;

        std::array<vk::CommandBuffer, FRAME_IN_FLIGHT> commandBuffers;
        std::array<vk::Semaphore, FRAME_IN_FLIGHT> imageAcquiredSemaphores;
        std::array<vk::Semaphore, FRAME_IN_FLIGHT> imageRenderedSemaphores;
        std::array<vk::Fence, FRAME_IN_FLIGHT> drawFences;
        int current = 0;

        void init(const vk::Device& device, const vk::CommandPool& commandPool) {
            for (int i = 0; i < FRAME_IN_FLIGHT; ++i) {
                imageAcquiredSemaphores[i] = device.createSemaphore({});
                imageRenderedSemaphores[i] = device.createSemaphore({});
                drawFences[i] = device.createFence({vk::FenceCreateFlagBits::eSignaled});
            }
            vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary,
                                                                    FRAME_IN_FLIGHT);
            device.allocateCommandBuffers(&commandBufferAllocateInfo, commandBuffers.data());
        }

        void destroy(const vk::Device& device, const vk::CommandPool& commandPool) {
            for (int i = 0; i < FRAME_IN_FLIGHT; ++i) {
                device.destroySemaphore(imageAcquiredSemaphores[i]);
                device.destroySemaphore(imageRenderedSemaphores[i]);
                device.destroyFence(drawFences[i]);
            }
            device.freeCommandBuffers(commandPool, commandBuffers);
        }

        const vk::Semaphore& imageAcquiredSemaphore() const { return imageAcquiredSemaphores[current]; }
        const vk::Semaphore& imageRenderedSemaphore() const { return imageRenderedSemaphores[current]; }
        const vk::Fence& drawFence() const { return drawFences[current]; }
        const vk::CommandBuffer& commandBuffer() const { return commandBuffers[current]; }

        void next() { current = (current + 1) % FRAME_IN_FLIGHT; }
    };
    FrameInFlight frame;
};
