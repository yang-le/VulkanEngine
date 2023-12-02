#pragma once

#include <bitset>

#include "vulkan.h"
#include <GLFW/glfw3.h>

#include "player.h"
#include "shader.h"
#include "settings.h"

// TODO: to be deleted
#include "utils.h"


struct Engine {
    Engine()
        : player(this), shader(this)
    {
        glfwInit();

        uint32_t count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        if (!extensions)
            throw std::runtime_error("GLFW cannot create Vulkan window surfaces!");

        vulkan.setInstanceExtensions({count, extensions});

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "VulkanEngine", nullptr, nullptr);

        vulkan.init(WIN_WIDTH, WIN_HEIGHT, [this](const vk::Instance& instance) {
            VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
            glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR);
            return surfaceKHR;
        });

        shader.load("cube");
        shader.write(coloredCubeData);
        shader.write("mvp", glm::mat4(1));
        shader.vert_format = {{vk::Format::eR32G32B32A32Sfloat, 0}, {vk::Format::eR32G32B32A32Sfloat, 16}};
        vulkan.attachShader(shader.vert_shader, shader.frag_shader, shader.vertex, shader.uniforms.begin()->second, shader.vert_format);

        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
            if (glfwGetWindowAttrib(window, GLFW_HOVERED) &&
                glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED &&
                button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
                glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                    Engine* self = (Engine*)glfwGetWindowUserPointer(window);

                    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                        glfwSetKeyCallback(window, nullptr);
                        glfwSetCursorPosCallback(window, nullptr);
                    }
                    else {
                        if (action == GLFW_PRESS)
                            self->key_state.set(key);
                        else if (action == GLFW_RELEASE)
                            self->key_state.reset(key);
                    }
                });
                glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
                    Engine* self = (Engine*)glfwGetWindowUserPointer(window);

                    auto prev_x = std::exchange(self->x, xpos);
                    auto prev_y = std::exchange(self->y, ypos);

                    self->dx += self->x - prev_x;
                    self->dy += self->y - prev_y;
                });
            }
        });
    }

    ~Engine() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            handle_events();
            update();
            render();
        }
    }

    void update() {
        player.update();
        shader.update();

        auto prev_t = std::exchange(t, glfwGetTime());
        dt = t - prev_t;

        fps = frame_count / dt;
        frame_count = 0;

        dx = dy = 0;
    }

    void render() {
        vulkan.draw();
        ++frame_count;
    }

    void handle_events() {
        player.handle_events();
    }

    Vulkan vulkan;
    Shader shader;
    GLFWwindow* window;
    Player player;
    float frame_count = 0, fps = 0;
    float t = 0, x = 0, y = 0;
    float dt = 0, dx = 0, dy = 0;
    std::bitset<GLFW_KEY_LAST> key_state;
};
