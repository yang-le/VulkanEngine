#pragma once

#include <stdint.h>

#include <bitset>

#include "settings.h"
#include "shader.h"

struct CloudMesh : Shader {
    CloudMesh(Engine& engine);

    virtual void init() override;
    virtual void update() override;

    using Vertex = glm::vec3;

    void gen_clouds();
    std::vector<Vertex> build_mesh();

    Engine& engine;
    std::bitset<WORLD_AREA * CHUNK_SIZE * CHUNK_SIZE> cloud_data;
};
