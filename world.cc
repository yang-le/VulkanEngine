#include "world.h"

#include "engine.h"

World::World(Engine* engine) : engine(engine) {
#ifdef _DEBUG
#pragma omp parallel for
#endif
    for (int x = 0; x < WORLD_W; ++x)
        for (int y = 0; y < WORLD_H; ++y)
            for (int z = 0; z < WORLD_D; ++z) {
                auto chunk = std::make_unique<ChunkMesh>(this, glm::vec3(x, y, z));
                int chunk_index = x + WORLD_W * z + WORLD_AREA * y;

                voxels[chunk_index] = chunk->build_voxels();
                chunk->voxels = &voxels[chunk_index];
                chunks[chunk_index] = std::move(chunk);
            }
}

void World::init() {
#ifdef _DEBUG
    for (auto& chunk : chunks)
        if (!chunk->empty) chunk->init();
#pragma omp parallel for
    for (auto& chunk : chunks)
        if (!chunk->empty) chunk->build_mesh();
#else
    for (auto& chunk : chunks)
        if (!chunk->empty) {
            chunk->init();
            chunk->build_mesh();
        }
#endif
}

void World::update() {
    for (auto& chunk : chunks)
        if (!chunk->empty /*&& chunk->is_on_frustum(engine->player) */) chunk->update();
}

void World::load() {
    for (auto& chunk : chunks)
        if (!chunk->empty) chunk->load();
}
