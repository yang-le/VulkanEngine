#include "engine.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>

#include "settings.h"

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
        // (1) ALWAYS forward mouse data to ImGui! This is automatic with default backends. With your own backend:
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, action);

        // (2) ONLY forward mouse data to your underlying app/game.
        if (io.WantCaptureMouse) return;

        if (glfwGetWindowAttrib(window, GLFW_HOVERED) &&
            glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED && button == GLFW_MOUSE_BUTTON_LEFT &&
            action == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                Engine* self = (Engine*)glfwGetWindowUserPointer(window);

                if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                    glfwSetKeyCallback(window, nullptr);
                    glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos) {
                        ImGuiIO& io = ImGui::GetIO();
                        io.AddMousePosEvent(xpos, ypos);
                    });
                } else if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
                    self->imgui_show = !self->imgui_show;
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
        } else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            Engine* self = (Engine*)glfwGetWindowUserPointer(window);
            self->handle_events(button, action);
        }
    });

    vk::DescriptorPoolSize pool_size = {vk::DescriptorType::eCombinedImageSampler, 1};
    imgui_pool = vulkan.device.createDescriptorPool({{}, 1, 1, &pool_size});

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkan.instance;
    init_info.PhysicalDevice = vulkan.physicalDevice;
    init_info.Device = vulkan.device;
    init_info.QueueFamily = vulkan.graphicsQueueFamliyIndex;
    init_info.Queue = vulkan.graphicsQueue;
    init_info.DescriptorPool = imgui_pool;
    init_info.MinImageCount = vulkan.imageCount;
    init_info.ImageCount = vulkan.imageCount;
    ImGui_ImplVulkan_Init(&init_info, vulkan.renderPass);
}

void Engine::run() {
    bool show_demo_window = false;
    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (imgui_show) {
            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::Begin("Information", nullptr, ImGuiWindowFlags_NoDecoration);
            auto prop = vulkan.physicalDevice.getProperties();
            ImGui::Text("Device: %s", prop.deviceName.data());
            ImGui::Text("Vulkan API Version: %d.%d.%d.%d", vk::apiVersionMajor(prop.apiVersion),
                        vk::apiVersionMinor(prop.apiVersion), vk::apiVersionPatch(prop.apiVersion),
                        vk::apiVersionVariant(prop.apiVersion));
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Chunk x: %d, y: %d, z: %d", (int)player.position.x / CHUNK_SIZE,
                        (int)player.position.y / CHUNK_SIZE, (int)player.position.z / CHUNK_SIZE);
            ImGui::Text("Player direction %d", (360 - (int)glm::degrees(player.yaw) % 360) % 360);
            ImGui::Text("Player position x: %d, y: %d, z: %d", (int)player.position.x, (int)player.position.y,
                        (int)player.position.z);
            ImGui::Text("Voxel position x: %d, y: %d, z: %d", (int)scene.world->voxel_handler->position.x,
                        (int)scene.world->voxel_handler->position.y, (int)scene.world->voxel_handler->position.z);
            ImGui::SliderFloat("Player speed", &PLAYER_SPEED, 0.01, 0.05);
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::End();
        }

        update();
        render();
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vulkan.device.destroyDescriptorPool(imgui_pool);
}

void Engine::update() {
    auto prev_t = std::exchange(t, glfwGetTime());
    dt = t - prev_t;

    player.update();
    scene.update();

    dx = dy = 0;
}

void Engine::render() {
    try {
        auto currentBuffer = vulkan.renderBegin();

        scene.draw();

        if (imgui_show) {
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized) ImGui_ImplVulkan_RenderDrawData(draw_data, vulkan.frame.commandBuffer());
        }

        vulkan.renderEnd(currentBuffer);
    } catch (const std::runtime_error& e) {
        if (!strcmp(e.what(), "resize")) {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }
            vulkan.resize({(uint32_t)width, (uint32_t)height});
        } else {
            throw;
        }
    };
}

void Engine::handle_events(int button, int action) { player.handle_events(button, action); }

void Engine::add_mesh(std::unique_ptr<Shader> mesh) { scene.add_mesh(std::move(mesh)); }
