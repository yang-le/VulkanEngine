#include <thread>

#include "vulkan.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "utils.h"


int main(int argc, char* argv[])
{
    glfwInit();

    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions) {
        puts("GLFW cannot create Vulkan window surfaces!");
        glfwTerminate();
        return 1;
    }

    Vulkan vulkan;
    vulkan.setInstanceExtensions({count, extensions});

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1600, 900, "VulkanEngine", nullptr, nullptr);

    if (!vulkan.init(1600, 900, vertexShaderText_PC_C, fragmentShaderText_C_C,
        sizeof(glm::mat4x4), sizeof(coloredCubeData), coloredCubeData, sizeof(coloredCubeData[0]),
        {{vk::Format::eR32G32B32A32Sfloat, 0}, {vk::Format::eR32G32B32A32Sfloat, 16}},
        [window](const vk::Instance& instance) -> vk::SurfaceKHR {
        VkSurfaceKHR surfaceKHR;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR))
            return nullptr;
        return surfaceKHR;
    })) {
        puts("Vulkan cannot initialize!");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glm::mat4x4 mvpcMatrix = createModelViewProjectionClipMatrix({1600, 900});
    vulkan.updateUniformBuffer(sizeof(glm::mat4x4), &mvpcMatrix);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        vulkan.draw();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
