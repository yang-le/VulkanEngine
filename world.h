#pragma once

#include "meshes/chunk_mesh.h"

struct Engine;
struct World : Shader {
    World(Engine* engine);

    virtual void init() override;
    virtual void update() override;
    virtual void attach() override;
    virtual void draw() override;

    Engine* engine;
    // can we sparse this?
    std::array<ChunkMesh::Voxels, WORLD_VOL> voxels;
    std::array<std::unique_ptr<ChunkMesh>, WORLD_VOL> chunks;
};
