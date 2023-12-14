#include "engine.h"

Engine::Engine() : player(this), scene(this) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "VulkanEngine", nullptr, nullptr);
}

Engine::~Engine() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::init() {
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions) throw std::runtime_error("GLFW cannot create Vulkan window surfaces!");

    vulkan.setInstanceExtensions({count, extensions});
    vulkan.setBackgroudColor({BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 1.0f});
    vulkan.init({WIN_WIDTH, WIN_HEIGHT}, [this](const vk::Instance& instance) {
        VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
        glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR);
        return surfaceKHR;
    });

    scene.init();

    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        if (glfwGetWindowAttrib(window, GLFW_HOVERED) &&
            glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED && button == GLFW_MOUSE_BUTTON_LEFT &&
            action == GLFW_PRESS) {
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
                } else {
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

void Engine::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        handle_events();
        update();
        render();
    }
}

void Engine::update() {
    auto prev_t = std::exchange(t, glfwGetTime());
    dt = t - prev_t;

    fps = 1 / dt;
    glfwSetWindowTitle(window, std::to_string(fps).c_str());

    player.update();
    scene.update();

    dx = dy = 0;
}

void Engine::render() {
    try {
        auto currentBuffer = vulkan.renderBegin();
        scene.draw();
        vulkan.renderEnd(currentBuffer);
    } catch (std::runtime_error e) {
        if (!strcmp(e.what(), "resize")) {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }
            vulkan.resize({(uint32_t)width, (uint32_t)height});
        } else {
            puts(e.what());
        }
    };
}

void Engine::handle_events() { player.handle_events(); }

void Engine::add_mesh(std::unique_ptr<Shader> mesh) { scene.add_mesh(std::move(mesh)); }
