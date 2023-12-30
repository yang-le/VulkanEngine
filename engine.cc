#include "engine.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>

Engine::Engine(uint32_t width, uint32_t height, uint32_t renderPassCount) : width(width), height(height) {
    start_time = std::chrono::system_clock::now();
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "VulkanEngine", nullptr, nullptr);

    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (!extensions) throw std::runtime_error("GLFW cannot create Vulkan window surfaces!");

    vulkan.setInstanceExtensions({count, extensions});
    vulkan.init(
        {width, height},
        [this](const vk::Instance& instance) {
            VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;
            glfwCreateWindowSurface(instance, window, nullptr, &surfaceKHR);
            return surfaceKHR;
        },
        renderPassCount + 1);

    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        // (1) ALWAYS forward mouse data to ImGui! This is automatic with default backends. With your own backend:
        ImGui::GetIO().AddMouseButtonEvent(button, action);

        // (2) ONLY forward mouse data to your underlying app/game.
        if (ImGui::GetIO().WantCaptureMouse) return;

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
                        io.AddMousePosEvent((float)xpos, (float)ypos);
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

                auto prev_x = std::exchange(self->mouse_x, (float)xpos);
                auto prev_y = std::exchange(self->mouse_y, (float)ypos);

                if (std::isinf(prev_x) || std::isinf(prev_y)) return;

                self->mouse_dx += self->mouse_x - prev_x;
                self->mouse_dy += self->mouse_y - prev_y;
            });
        } else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            Engine* self = (Engine*)glfwGetWindowUserPointer(window);
            self->handle_events(button, action);
        }
    });

    // init scene
    if (!player) player = std::make_unique<Player>(*this);
    if (!scene) scene = std::make_unique<Scene>();

    // init for imgui
    vk::DescriptorPoolSize pool_size = {vk::DescriptorType::eCombinedImageSampler, 1};
    imgui_pool =
        vulkan.device.createDescriptorPool({vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, 1, &pool_size});

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);

    add_gui(std::make_unique<EngineGui>(*this));
}

Engine::~Engine() {
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vulkan.device.destroyDescriptorPool(imgui_pool);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::run() {
    scene->init();

    // Setup Platform/Renderer backends
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkan.instance;
    init_info.PhysicalDevice = vulkan.physicalDevice;
    init_info.Device = vulkan.device;
    init_info.QueueFamily = vulkan.graphicsQueueFamliyIndex;
    init_info.Queue = vulkan.graphicsQueue;
    init_info.DescriptorPool = imgui_pool;
    init_info.MinImageCount = vulkan.minImageCount;
    init_info.ImageCount = vulkan.minImageCount;

    vulkan.addRenderPass({}, true);
    ImGui_ImplVulkan_Init(&init_info, vulkan.renderPass());

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }

    vulkan.device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
}

void Engine::render() {
    // update dt
    auto prev_t = std::exchange(t, std::chrono::system_clock::now());
    dt = t - prev_t;

    player->update();
    scene->update();

    // clear mouse_dx, mouse_dy for next cycle
    mouse_dx = mouse_dy = 0;

    try {
        vulkan.renderBegin();

        scene->draw();

        if (imgui_show) {
            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            for (auto& gui : guis) gui->gui_render();
            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized) {
                vulkan.nextPass();
                ImGui_ImplVulkan_RenderDrawData(draw_data, vulkan.frame.commandBuffer());
            }
        }

        vulkan.renderEnd();
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

void Engine::EngineGui::gui_draw() {
    static bool show_demo_window = false;
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    auto prop = engine.vulkan.physicalDevice.getProperties();
    ImGui::Text("Device: %s", prop.deviceName.data());
    ImGui::Text("Vulkan API Version: %d.%d.%d.%d", vk::apiVersionMajor(prop.apiVersion),
                vk::apiVersionMinor(prop.apiVersion), vk::apiVersionPatch(prop.apiVersion),
                vk::apiVersionVariant(prop.apiVersion));
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    // ImGui::Text("Chunk x: %d, y: %d, z: %d", (int)engine.player->position.x / CHUNK_SIZE,
    //             (int)engine.player->position.y / CHUNK_SIZE, (int)engine.player->position.z / CHUNK_SIZE);
    ImGui::Text("Player direction %d", (360 - (int)glm::degrees(engine.player->yaw) % 360) % 360);
    ImGui::Text("Player pitch %d", (int)glm::degrees(engine.player->pitch));
    ImGui::Text("Player position x: %d, y: %d, z: %d", (int)engine.player->position.x, (int)engine.player->position.y,
                (int)engine.player->position.z);
    // ImGui::Text("Voxel position x: %d, y: %d, z: %d", (int)engine.scene->world->voxel_handler->position.x,
    //             (int)engine.scene->world->voxel_handler->position.y,
    //             (int)engine.scene->world->voxel_handler->position.z);
    ImGui::SliderFloat("Player speed", &engine.player->speed, 1, 100);
    ImGui::Checkbox("Demo Window", &show_demo_window);
}
