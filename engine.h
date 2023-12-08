#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <bitset>

#include "meshes/cloud_mesh.h"
#include "meshes/water_mesh.h"
#include "player.h"
#include "settings.h"
#include "vulkan.h"

struct Engine {
    Engine();
    ~Engine();

    void run();
    void update();
    void render();
    void handle_events();

    Vulkan vulkan;
    GLFWwindow* window;
    Player player;
    float fps = 0;
    float t = 0, x = 0, y = 0;
    float dt = 0, dx = 0, dy = 0;
    std::bitset<GLFW_KEY_LAST> key_state;

    std::vector<std::unique_ptr<Shader>> meshes;
};
