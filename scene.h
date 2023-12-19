#pragma once

#include "shader.h"

struct Scene {
    virtual ~Scene() = default;

    virtual void init();
    virtual void update();
    virtual void draw();

    void add_mesh(std::unique_ptr<Shader> shader);

    std::vector<std::unique_ptr<Shader>> meshes;

   protected:
    virtual void load() {}
    virtual void attach() {}
};
