#pragma once

#include <stdint.h>

#include "shader.h"

struct WaterMesh : Shader {
    WaterMesh(Engine& engine);

    virtual void init() override;

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };
};
