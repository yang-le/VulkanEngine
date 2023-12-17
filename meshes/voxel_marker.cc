#include "meshes/voxel_marker.h"

#include "engine.h"
#include "world.h"

namespace {
inline int get_chunk_index(int wx, int wy, int wz) {
    int cx = wx / CHUNK_SIZE;
    int cy = wy / CHUNK_SIZE;
    int cz = wz / CHUNK_SIZE;
    if (wx < 0 || cx >= WORLD_W || wy < 0 || cy >= WORLD_H || wz < 0 || cz >= WORLD_D) return -1;

    return cx + WORLD_W * cz + WORLD_AREA * cy;
}
}  // namespace

VoxelMarkerMesh::VoxelMarkerMesh(World* world)
    : Shader("voxel_marker", world->engine), world(world), engine(world->engine) {
    position = glm::vec3(0);
    vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
}

void VoxelMarkerMesh::init() {
    Shader::init();

    constexpr std::array<std::tuple<float, float, float>, 8> vertices = {std::tuple<float, float, float>{0, 0, 1},
                                                                         {1, 0, 1},
                                                                         {1, 1, 1},
                                                                         {0, 1, 1},
                                                                         {0, 1, 0},
                                                                         {0, 0, 0},
                                                                         {1, 0, 0},
                                                                         {1, 1, 0}};
    constexpr std::array<size_t, 36> indices = {0, 2, 3, 0, 1, 2, 1, 7, 2, 1, 6, 7, 6, 5, 4, 4, 7, 6,
                                                3, 4, 5, 3, 5, 0, 3, 7, 4, 3, 2, 7, 0, 6, 1, 0, 5, 6};

    constexpr std::array<std::tuple<float, float>, 4> tex_coord_vertices = {
        std::tuple<float, float>{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    constexpr std::array<size_t, 36> tex_coord_indices = {0, 2, 3, 0, 1, 2, 0, 2, 3, 0, 1, 2, 0, 1, 2, 2, 3, 0,
                                                          2, 3, 0, 2, 0, 1, 0, 2, 3, 0, 1, 2, 3, 1, 2, 3, 0, 1};

    auto vertex_data = hstack<Vertex>(get_data(vertices, indices), get_data(tex_coord_vertices, tex_coord_indices));
    for (auto& vertex : vertex_data) vertex.pos = (vertex.pos - 0.5f) * 1.01f + 0.5f;
    write_vertex(vertex_data);

    write_uniform(2, glm::mat4(1));
    write_uniform(3, interaction_mode);
    write_texture(4, "frame.png");
}

void VoxelMarkerMesh::update() {
    Shader::update();
    ray_cast();
    if (voxel_id) {
        if (interaction_mode)
            position = voxel_world_pos + voxel_normal;
        else
            position = voxel_world_pos;
    }
    write_uniform(2, glm::translate(glm::mat4(1), position));
    write_uniform(3, interaction_mode);
}

void VoxelMarkerMesh::draw() {
    if (voxel_id) Shader::draw();
}

void VoxelMarkerMesh::add_voxel() {
    if (!voxel_id) return;

    // check voxel id along normal
    auto result = get_voxel_info(voxel_world_pos + voxel_normal);
    if (!result.chunk) return;

    // is the new place empty?
    if (!result.id) {
        auto chunk = result.chunk;
        chunk->voxels->at(result.index) = new_voxel_id;
        rebuild_adj_chunks();

        // was it an empty chunk?
        if (!chunk->empty) [[likely]]
            chunk->rebuild_mesh();
        else {
            chunk->empty = false;
            chunk->init();
            chunk->attach();
        }
    }
}

void VoxelMarkerMesh::rebuild_adj_chunk(int wx, int wy, int wz) {
    auto index = get_chunk_index(wx, wy, wz);
    if (index != -1) world->chunks[index]->rebuild_mesh();
}

void VoxelMarkerMesh::rebuild_adj_chunks() {
    int lx = voxel_local_pos.x, ly = voxel_local_pos.y, lz = voxel_local_pos.z;
    int wx = voxel_world_pos.x, wy = voxel_world_pos.y, wz = voxel_world_pos.z;

    if (lx == 0)
        rebuild_adj_chunk(wx - 1, wy, wz);
    else if (lx == CHUNK_SIZE - 1)
        rebuild_adj_chunk(wx + 1, wy, wz);

    if (ly == 0)
        rebuild_adj_chunk(wx, wy - 1, wz);
    else if (ly == CHUNK_SIZE - 1)
        rebuild_adj_chunk(wx, wy + 1, wz);

    if (lz == 0)
        rebuild_adj_chunk(wx, wy, wz - 1);
    else if (lz == CHUNK_SIZE - 1)
        rebuild_adj_chunk(wx, wy, wz + 1);
}

void VoxelMarkerMesh::remove_voxel() {
    if (!voxel_id) return;

    chunk->voxels->at(voxel_index) = 0;
    rebuild_adj_chunks();
    chunk->rebuild_mesh();

    // was it an empty chunk?
    chunk->empty = !std::any_of(chunk->voxels->begin(), chunk->voxels->end(), [](uint8_t id) { return id != 0; });
}

void VoxelMarkerMesh::set_voxel() {
    if (interaction_mode)
        add_voxel();
    else
        remove_voxel();
}

void VoxelMarkerMesh::switch_mode() { interaction_mode = !interaction_mode; }

bool VoxelMarkerMesh::ray_cast() {
    auto ray = engine->player.forward * (float)MAX_RAY_DIST;

    voxel_id = 0;
    voxel_normal = glm::ivec3(0);

    auto step_dir = -1;
    auto current_voxel_pos = glm::ivec3(engine->player.position);

    auto sign = glm::sign(ray);
    auto delta = glm::min(sign / ray, 10000000.0f);
    auto dx = sign.x != 0 ? delta.x : 10000000.0f;
    auto dy = sign.y != 0 ? delta.y : 10000000.0f;
    auto dz = sign.z != 0 ? delta.z : 10000000.0f;

    auto fract = glm::fract(engine->player.position);
    auto max_x = sign.x > 0 ? dx * (1.0 - fract.x) : dx * fract.x;
    auto max_y = sign.y > 0 ? dy * (1.0 - fract.y) : dy * fract.y;
    auto max_z = sign.z > 0 ? dz * (1.0 - fract.z) : dz * fract.z;

    while (max_x <= 1.0 || max_y <= 1.0 || max_z <= 1.0) {
        auto result = get_voxel_info(current_voxel_pos);
        if (result.id) {
            voxel_id = result.id;
            voxel_index = result.index;
            voxel_local_pos = result.pos;
            chunk = result.chunk;
            voxel_world_pos = current_voxel_pos;

            if (step_dir == 0)
                voxel_normal.x = -sign.x;
            else if (step_dir == 1)
                voxel_normal.y = -sign.y;
            else
                voxel_normal.z = -sign.z;

            return true;
        }
        if (max_x < max_y) {
            if (max_x < max_z) {
                current_voxel_pos.x += sign.x;
                max_x += dx;
                step_dir = 0;
            } else {
                current_voxel_pos.z += sign.z;
                max_z += dz;
                step_dir = 2;
            }
        } else {
            if (max_y < max_z) {
                current_voxel_pos.y += sign.y;
                max_y += dy;
                step_dir = 1;
            } else {
                current_voxel_pos.z += sign.z;
                max_z += dz;
                step_dir = 2;
            }
        }
    }

    return false;
}

VoxelMarkerMesh::VoxelInfo VoxelMarkerMesh::get_voxel_info(glm::ivec3 voxel_world_pos) {
    auto chunk_pos = voxel_world_pos / CHUNK_SIZE;
    int cx = chunk_pos.x, cy = chunk_pos.y, cz = chunk_pos.z;
    if (cx < 0 || cx >= WORLD_W || cy < 0 || cy >= WORLD_H || cz < 0 || cz >= WORLD_D) return {};

    auto chunk_index = cx + WORLD_W * cz + WORLD_AREA * cy;
    auto& chunk = world->chunks[chunk_index];

    auto voxel_local_pos = voxel_world_pos - chunk_pos * CHUNK_SIZE;
    auto voxel_index = voxel_local_pos.x + CHUNK_SIZE * voxel_local_pos.z + CHUNK_AREA * voxel_local_pos.y;
    if (voxel_index < 0) return {};

    auto voxel_id = chunk->voxels->at(voxel_index);
    return {voxel_id, voxel_index, voxel_local_pos, chunk.get()};
}
