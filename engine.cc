#include "engine.h"

// TODO: to be deleted
#include "utils.h"
#include <vector>


Engine::Engine()
    : player(this), water_mesh(this)
{
    glfwInit();

    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions)
        throw std::runtime_error("GLFW cannot create Vulkan window surfaces!");

    vulkan.setInstanceExtensions({count, extensions});

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "VulkanEngine", nullptr, nullptr);

    vulkan.init({WIN_WIDTH, WIN_HEIGHT}, [this](const vk::Instance& instance) {
        VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
        glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR);
        return surfaceKHR;
    });

    water_mesh.init();

    glslang::InitializeProcess();
    water_mesh.load("water");
    glslang::FinalizeProcess();

    std::vector<Vulkan::Buffer> uniforms;
    std::vector<Vulkan::Texture> textures;
    for (auto& it : water_mesh.uniforms)
        uniforms.push_back(it.second);
    for (auto& it : water_mesh.textures)
        textures.push_back(it.second);

    vulkan.attachShader(water_mesh.vert_shader, water_mesh.frag_shader, water_mesh.vertex, water_mesh.vert_format, uniforms, textures);

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

Engine::~Engine() {
    glfwDestroyWindow(window);
    glfwTerminate();
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
    player.update();
    water_mesh.update();

    auto prev_t = std::exchange(t, glfwGetTime());
    dt = t - prev_t;

    fps = frame_count / dt;
    frame_count = 0;

    dx = dy = 0;
}

void Engine::render() {
    vulkan.draw();
    ++frame_count;
}

void Engine::handle_events() {
    player.handle_events();
}
