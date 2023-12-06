#pragma once

#include <stdint.h>

#include "shader.h"

struct WaterMesh : Shader {
    WaterMesh(Engine* engine);

    virtual void init() override;

    struct Vertex {
        float x, y, z;
        float u, v;
    };
};
