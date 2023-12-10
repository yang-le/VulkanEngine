#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <bitset>

#include "player.h"
#include "scene.h"
#include "settings.h"
#include "vulkan.h"

struct Engine {
    Engine();
    ~Engine();

    void init();
    void run();
    void update();
    void render();
    void handle_events();
    void add_mesh(std::unique_ptr<Shader> mesh);

    Vulkan vulkan;
    GLFWwindow* window;
    Scene scene;
    Player player;
    float fps = 0;
    float t = 0, x = 0, y = 0;
    float dt = 0, dx = 0, dy = 0;
    std::bitset<GLFW_KEY_LAST> key_state;
};
