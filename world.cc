#include "world.h"

#include "engine.h"

World::World(Engine* engine) : engine(engine), Shader("chunk", engine) {
#ifdef _DEBUG
#pragma omp parallel for
#endif
    for (int x = 0; x < WORLD_W; ++x)
        for (int y = 0; y < WORLD_H; ++y)
            for (int z = 0; z < WORLD_D; ++z) {
                auto chunk = std::make_unique<ChunkMesh>(this, glm::vec3(x, y, z));
                int chunk_index = x + WORLD_W * z + WORLD_AREA * y;

                voxels[chunk_index] = chunk->build_voxels();
                chunk->voxels = voxels[chunk_index].get();
                chunks[chunk_index] = std::move(chunk);
            }
    voxel_handler = std::make_unique<VoxelMarkerMesh>(this);
}

void World::init() {
    Shader::init();
    write_uniform(3, BG_COLOR, vk::ShaderStageFlagBits::eFragment);

#ifdef _DEBUG
#pragma omp parallel for
#endif
    for (auto& chunk : chunks)
        if (!chunk->empty) chunk->init();

    write_texture(4,
                  {"sand.png", "dirt.png", "grass_block_side.png", "grass_block_top.png", "stone.png", "snow.png",
                   "birch_leaves.png", "birch_log.png", "birch_log_top.png"},
                  {0, 0, 0, 1, 2, 3, 1, 1, 1, 4, 4, 4, 5, 5, 5, 6, 6, 6, 8, 7, 8});
    voxel_handler->init();
}

void World::update() {
    for (auto& chunk : chunks)
        if (!chunk->empty && chunk->is_on_frustum(engine->player)) chunk->update();
    voxel_handler->update();
}

void World::load() {
    Shader::load();
    voxel_handler->load();
}

void World::attach() {
    for (auto& chunk : chunks)
        if (!chunk->empty) chunk->attach();
    voxel_handler->attach();

    vulkan->destroyShaderModule(frag_shader);
    vulkan->destroyShaderModule(vert_shader);
}

void World::draw() {
    for (auto& chunk : chunks)
        if (!chunk->empty && chunk->is_on_frustum(engine->player)) chunk->draw();
    voxel_handler->draw();
}
