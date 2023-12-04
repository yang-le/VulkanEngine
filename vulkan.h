#pragma once

#include <functional>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>


class Vulkan {
public:
    struct Buffer {
        vk::Buffer buffer = {};
        VmaAllocation memory = {};
        void* data = nullptr;

        size_t size = 0;
        size_t stride = 0;
    };

    struct Texture {
        vk::Image image = {};
        vk::ImageView view = {};
        VmaAllocation memory = {};
        vk::Sampler sampler = {};

        vk::Buffer stagingBuffer = {};
        VmaAllocation stagingMemory = {};
    };

    Vulkan() = default;
    ~Vulkan();

    Vulkan& setAppInfo(const char* appName, uint32_t appVerson = {});
    Vulkan& setEngineInfo(const char* engineName, uint32_t engineVersion = {});
    Vulkan& setApiVersion(uint32_t apiVersion);
    Vulkan& setInstanceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers);
    Vulkan& setInstanceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions);
    Vulkan& setDeviceLayers(const vk::ArrayProxyNoTemporaries<const char *const>& layers);
    Vulkan& setDeviceExtensions(const vk::ArrayProxyNoTemporaries<const char *const>& extensions);
    Vulkan& setDeviceFeatures(const vk::PhysicalDeviceFeatures& features);

    void init(vk::Extent2D extent, std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR, std::function<bool(const vk::PhysicalDevice &)> pickDevice = {});
    void attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule, const Buffer& vertex, const std::vector<std::pair<vk::Format, uint32_t>>& vertexInputeAttributeFormatOffset, const std::vector<Buffer>& uniforms, const std::vector<Texture>& textures);
    void draw();

    Buffer createUniformBuffer(vk::DeviceSize size);
    void destroyUniformBuffer(const Buffer& buffer);

    template<typename T, size_t Size>
    Buffer createVertexBuffer(const T (&vertices)[Size]) {
        Buffer buffer;
        buffer.stride = sizeof(T);
        buffer.size = sizeof(T) * Size;

        std::tie(buffer.buffer, buffer.memory) = createBuffer(buffer.size, vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vmaMapMemory(vmaAllocator, buffer.memory, &buffer.data);
        memcpy(buffer.data, vertices, buffer.size);
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
    void enumerateDevice(std::function<bool(const vk::PhysicalDevice &)> pickDevice);
    void initDevice(std::function<vk::SurfaceKHR(const vk::Instance &)> getSurfaceKHR);
    void initCommandBuffer();
    void initSwapChain(vk::Extent2D extent);
    void initDepthBuffer();
    void initPipelineLayout();
    void initDescriptorSet();
    void initRenderPass();
    void initFrameBuffers();
    void initPipeline(const vk::ShaderModule& vertexShaderModule, const vk::ShaderModule& fragmentShaderModule, uint32_t vertexStride, const std::vector<std::pair<vk::Format, uint32_t>>& vertexInputeAttributeFormatOffset, bool depthBuffered = true);


    //
    // Utils
    //

    vk::SurfaceFormatKHR pickSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    std::pair<vk::Buffer, VmaAllocation> createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags bufferProp = {});
    std::pair<vk::Image, VmaAllocation> createImage(vk::Extent2D extent, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageLayout layout = vk::ImageLayout::eUndefined);
    void setImageLayout(const vk::CommandBuffer& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);


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
    vk::Extent2D imageExtent;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::PipelineLayout pipelineLayout;
    std::array<vk::DescriptorSetLayout, 2> descriptorSetLayout;
    std::array<vk::DescriptorPool, 2> descriptorPool;
    std::array<vk::DescriptorSet, 2> descriptorSets;
    vk::Image depthImage;
    VmaAllocation depthMemory;
    vk::ImageView depthImageView;
    Buffer vertexBuffer;
    std::vector<Buffer> uniformBuffer;
    std::vector<Texture> textures;
    std::vector<vk::Framebuffer> framebuffers;
    vk::RenderPass renderPass;
    vk::Pipeline graphicsPipeline;
    vk::Semaphore imageAcquiredSemaphore;
    vk::Fence drawFence;
};
