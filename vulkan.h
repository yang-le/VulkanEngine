#pragma once

#include <functional>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <vk_mem_alloc.h>

class Vulkan {
   public:
    struct Buffer {
        vk::Buffer buffer = {};
        VmaAllocation memory = {};
        void* data = nullptr;

        size_t size = 0;
        size_t stride = 0;
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
    Vulkan& setBackgroudColor(const vk::ClearColorValue& value);

    void init(vk::Extent2D extent, std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR,
              std::function<bool(const vk::PhysicalDevice&)> pickDevice = {});
    void attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule, const Buffer& vertex,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Buffer>& uniforms,
                      const std::map<int, Texture>& textures, vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack);
    void draw();
    void resize(vk::Extent2D extent);

    Buffer createUniformBuffer(vk::DeviceSize size);
    void destroyUniformBuffer(const Buffer& buffer);

    template <typename T, size_t Size>
    Buffer createVertexBuffer(const T (&vertices)[Size]) {
        Buffer buffer;
        buffer.stride = sizeof(T);
        buffer.size = sizeof(T) * Size;

        std::tie(buffer.buffer, buffer.memory) =
            createBuffer(buffer.size, vk::BufferUsageFlagBits::eVertexBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
        memcpy(buffer.data, vertices, buffer.size);
        vmaUnmapMemory(vmaAllocator, buffer.memory);

        return buffer;
    }
    template <typename T>
    Buffer createVertexBuffer(const std::vector<T>& vertices) {
        Buffer buffer;
        buffer.stride = sizeof(T);
        buffer.size = sizeof(T) * vertices.size();

        std::tie(buffer.buffer, buffer.memory) =
            createBuffer(buffer.size, vk::BufferUsageFlagBits::eVertexBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
        memcpy(buffer.data, vertices.data(), buffer.size);
        vmaUnmapMemory(vmaAllocator, buffer.memory);

        return buffer;
    }
    void destroyVertexBuffer(const Buffer& buffer);

    Texture createTexture(vk::Extent2D extent, const void* data, bool anisotropy = false);
    void destroyTexture(const Texture& texture);

    vk::ShaderModule createShaderModule(vk::ShaderStageFlagBits shaderStage, const std::string& shaderText);
    void destroyShaderModule(const vk::ShaderModule& shader);

   private:
    void initInstance();
    void enumerateDevice(std::function<bool(const vk::PhysicalDevice&)> pickDevice);
    void initDevice(std::function<vk::SurfaceKHR(const vk::Instance&)> getSurfaceKHR);
    void initCommandBuffer();
    void initSwapChain(vk::Extent2D extent);
    void initDepthBuffer();
    void initRenderPass();
    void initFrameBuffers();
    void initDescriptorSet(const std::map<int, Buffer>& uniforms, const std::map<int, Texture>& textures);
    void initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule,
                      uint32_t vertexStride, const std::vector<vk::Format>& vertexFormats, vk::CullModeFlags cullMode,
                      bool depthBuffered = true);
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
                                                    uint32_t mipLevels = 1);
    void setImageLayout(const vk::CommandBuffer& commandBuffer, vk::Image image, vk::Format format,
                        vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevel = 0,
                        uint32_t mipCount = 1);

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
#define computeQueue graphicsQueue
    VmaAllocator vmaAllocator;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::Extent2D imageExtent;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::DescriptorPool> descriptorPools;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::Image depthImage;
    VmaAllocation depthMemory;
    vk::ImageView depthImageView;
    std::vector<Buffer> vertexBuffers;
    std::vector<vk::Framebuffer> framebuffers;
    vk::RenderPass renderPass;
    std::vector<vk::PipelineLayout> pipelineLayouts;
    std::vector<vk::Pipeline> graphicsPipelines;
    vk::ClearColorValue bgColor;

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
