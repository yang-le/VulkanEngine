#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <bitset>

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
    WaterMesh water_mesh;
    GLFWwindow* window;
    Player player;
    float frame_count = 0, fps = 0;
    float t = 0, x = 0, y = 0;
    float dt = 0, dx = 0, dy = 0;
    std::bitset<GLFW_KEY_LAST> key_state;
};
