#pragma once

#include "engine.h"
#include "meshes/chunk_mesh.h"
#include "meshes/voxel_marker.h"

struct World : Shader {
    World(Engine& engine);

    virtual void init() override;
    virtual void update() override;
    virtual void load() override;
    virtual void attach() override;
    virtual void draw() override;

    const Camera& camera;
    // can we sparse this?
    std::array<std::unique_ptr<ChunkMesh::Voxels>, WORLD_VOL> voxels;
    std::array<std::unique_ptr<ChunkMesh>, WORLD_VOL> chunks;
    std::unique_ptr<VoxelMarkerMesh> voxel_handler;
};
