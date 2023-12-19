#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <bitset>

#include "gui.h"
#include "player.h"
#include "scene.h"
#include "vulkan.h"

struct Engine {
    Engine();

    void run();
    void add_mesh(std::unique_ptr<Shader> mesh);
    void add_gui(std::unique_ptr<Gui> gui);

    Vulkan vulkan;
    GLFWwindow* window;
    Scene scene;
    Player player;
    std::vector<std::unique_ptr<Gui>> guis;

    float t = 0, x = 0, y = 0;
    float dt = 0, dx = 0, dy = 0;
    std::bitset<GLFW_KEY_LAST> key_state;

    bool imgui_show = false;
    vk::DescriptorPool imgui_pool;

    struct EngineGui : Gui {
        EngineGui(Engine* engine) : engine(engine), Gui("Information") {}
        virtual void gui_draw() override;
        Engine* engine;
    };

   private:
    void init();
    void destroy();
    void render();
    void handle_events(int button, int action);
};
