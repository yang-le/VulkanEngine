#pragma once

#include "shader.h"
#include <stdint.h>


struct WaterMesh : Shader {
    WaterMesh(Engine* engine);

    virtual void init() override;

    struct Vertex {
        uint8_t u, v;
        uint8_t x, y, z;
    };
};
