#pragma once

#include "shader.h"

class Engine;

struct Scene {
    Scene(Engine* engine);

    void init();
    void update();
    void draw();

    void add_mesh(std::unique_ptr<Shader> shader);

    std::vector<std::unique_ptr<Shader>> meshes;
    Engine* engine;
    Vulkan* vulkan;
};
