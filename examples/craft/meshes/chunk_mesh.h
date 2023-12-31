#pragma once

#include "settings.h"
#include "shader.h"

struct World;
struct ChunkMesh : Shader {
    ChunkMesh() = default;
    ChunkMesh(Engine& engine, World* world, glm::vec3 pos);
    virtual ~ChunkMesh() override;

    virtual void init() override;
    virtual void attach(uint32_t subpass = 0) override;

    using Vertex = uint32_t;
    using Voxels = std::array<uint8_t, CHUNK_VOL>;

    std::unique_ptr<Voxels> build_voxels();
    std::vector<Vertex> build_mesh();
    void rebuild_mesh();
    bool is_on_frustum(const Camera& camera);

    World* world;
    bool empty = true;
    glm::vec3 position;
    glm::vec3 center;
    glm::mat4 model;
    Voxels* voxels;
};
