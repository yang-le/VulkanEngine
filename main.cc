#include <print>

#include "vulkan.h"
#include <GLFW/glfw3.h>


int main(int argc, char* argv[])
{
    glfwInit();

    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions) {
        std::println("GLFW cannot create Vulkan window surfaces!");
        glfwTerminate();
        return 1;
    }

    Vulkan vulkan;
    vulkan.setInstanceExtensions({count, extensions});

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1600, 900, "VulkanEngine", nullptr, nullptr);

    if (!vulkan.init(1600, 900, [window](const vk::Instance& instance) -> vk::SurfaceKHR {
        VkSurfaceKHR surfaceKHR;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR))
            return nullptr;
        return surfaceKHR;
    })) {
        std::println("Vulkan cannot initialize!");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
