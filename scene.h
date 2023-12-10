#pragma once

#include "shader.h"
#include "world.h"

class Engine;

struct Scene {
    Scene(Engine* engine);

    void init();
    void update();

    void add_mesh(std::unique_ptr<Shader> shader);

    std::vector<std::unique_ptr<Shader>> meshes;
    std::unique_ptr<World> world;
    Engine* engie;
    Vulkan* vulkan;
};
