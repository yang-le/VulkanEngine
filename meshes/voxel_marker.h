#pragma once

#include "meshes/chunk_mesh.h"

struct World;
struct VoxelMarkerMesh : Shader {
    VoxelMarkerMesh(Engine& engine, const World& world);

    virtual void init() override;
    virtual void update() override;
    virtual void draw() override;

    void add_voxel();
    void rebuild_adj_chunk(int wx, int wy, int wz);
    void rebuild_adj_chunks();
    void remove_voxel();
    void set_voxel();
    void switch_mode();
    bool ray_cast();

    struct VoxelInfo {
        uint8_t id;
        int index;
        glm::ivec3 pos;
        ChunkMesh* chunk;
    };

    VoxelInfo get_voxel_info(glm::ivec3 voxel_world_pos);

    struct Vertex {
        Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), uv(u, v) {}
        glm::vec3 pos;
        glm::vec2 uv;
    };

    const World& world;
    const Camera& camera;

    // ray casting result
    ChunkMesh* chunk;
    uint8_t voxel_id;
    size_t voxel_index;
    glm::ivec3 voxel_local_pos;
    glm::ivec3 voxel_world_pos;
    glm::ivec3 voxel_normal;

    glm::vec3 position;
    uint32_t interaction_mode = 0;
    uint8_t new_voxel_id = 1;
};
