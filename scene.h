#pragma once

#include "shader.h"
#include "world.h"

struct Engine;
struct Scene {
    Scene(Engine* engine);

    void init();
    void update();
    void draw();

    void add_mesh(std::unique_ptr<Shader> shader);

    std::unique_ptr<World> world;
    std::vector<std::unique_ptr<Shader>> meshes;

    Engine* engine;
    Vulkan* vulkan;
};
