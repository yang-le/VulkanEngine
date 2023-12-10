#pragma once

#include "shader.h"

struct World;
struct ChunkMesh : Shader {
    ChunkMesh() = default;
    ChunkMesh(World* engine, glm::vec3 pos);

    virtual void init() override;

    using Vertex = uint32_t;
    using Voxels = std::array<uint8_t, CHUNK_VOL>;

    Voxels build_voxels();
    void build_mesh();
    bool is_on_frustum(const Camera& camera);

    World* world;
    bool empty = true;
    glm::vec3 position;
    glm::vec3 center;
    glm::mat4 model;
    Voxels* voxels;
};
