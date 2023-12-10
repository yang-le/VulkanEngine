#pragma once

#include "meshes/chunk_mesh.h"

struct Engine;
struct World {
    World(Engine* engine);

    void init();
    void update();
    void load();

    Engine* engine;
    // can we sparse this?
    std::array<ChunkMesh::Voxels, WORLD_VOL> voxels;
    std::array<std::unique_ptr<ChunkMesh>, WORLD_VOL> chunks;
};
