#include "chunk_mesh.h"

#include <glm/gtc/noise.hpp>
#include <random>

#include "world.h"

namespace {
int get_height(glm::vec2 pos) {
    // amplitude
    float a1 = CENTER_Y;
    float a2 = a1 * 0.5f, a4 = a1 * 0.25f, a8 = a1 * 0.125f;

    // frequency
    float f1 = 0.005f;
    float f2 = f1 * 2, f4 = f1 * 4, f8 = f1 * 8;

    if (glm::simplex(0.1f * pos) < 0) a1 /= 1.07f;

    float height = 0;
    height += glm::simplex(f1 * pos) * a1 + a1;
    height += glm::simplex(f2 * pos) * a2 - a2;
    height += glm::simplex(f4 * pos) * a4 + a4;
    height += glm::simplex(f8 * pos) * a8 - a8;
    height = std::max(height, 1.0f);

    // island mask
    float island = 1.0f / (std::pow(0.0025f * (std::hypot(pos.x - CENTER_XZ, pos.y - CENTER_XZ)), 20.0f) + 0.0001f);
    height *= std::min(island, 1.0f);

    return (int)height;
}

inline int get_index(int x, int y, int z) { return x + CHUNK_SIZE * z + CHUNK_AREA * y; }

void place_tree(std::array<uint8_t, CHUNK_VOL>& voxels, int x, int y, int z, int voxel_id) {
    static std::random_device r;
    static std::default_random_engine e(r());
    static std::uniform_real_distribution<float> uniform_dist;

    if (voxel_id != GRASS || uniform_dist(e) > TREE_PROBABILITY) return;
    if (y + TREE_HEIGHT >= CHUNK_SIZE) return;
    if (x - TREE_H_WIDTH < 0 || x + TREE_H_WIDTH >= CHUNK_SIZE) return;
    if (z - TREE_H_WIDTH < 0 || z + TREE_H_WIDTH >= CHUNK_SIZE) return;

    // dirt under the tree
    voxels[get_index(x, y, z)] = DIRT;

    // leaves
    int m = 0;
    for (int iy = TREE_H_HEIGHT; iy < TREE_HEIGHT - 1; ++iy) {
        static std::uniform_real_distribution<float> uniform_dist(0, 2);

        int k = iy % 2;
        int rng = (int)uniform_dist(e);
        for (int ix = -TREE_H_WIDTH + m; ix < TREE_H_WIDTH - m * rng; ++ix)
            for (int iz = -TREE_H_WIDTH + m * rng; iz < TREE_H_WIDTH - m; ++iz)
                if ((ix + iz) % 4) voxels[get_index(x + ix + k, y + iy, z + iz + k)] = LEAVES;
        int n = iy - TREE_H_HEIGHT;
        m += n > 1 ? 3 : n > 0 ? 1 : 0;
    }

    // tree trunk
    for (int iy = 1; iy < TREE_HEIGHT - 2; ++iy) voxels[get_index(x, y + iy, z)] = WOOD;

    // top
    voxels[get_index(x, y + TREE_HEIGHT - 2, z)] = LEAVES;
}

void set_voxel_id(std::array<uint8_t, CHUNK_VOL>& voxels, int x, int y, int z, int wx, int wy, int wz,
                  int world_height) {
    int voxel_id = 0;

    if (wy < world_height - 1)
        // create caves
        if (glm::simplex(glm::vec3(wx * 0.09, wy * 0.09, wz * 0.09)) > 0 &&
            glm::simplex(glm::vec2(wx * 0.1, wz * 0.1)) * 3 + 3 < wy && wy < world_height - 10)
            voxel_id = 0;
        else
            voxel_id = STONE;
    else {
        static std::random_device r;
        static std::default_random_engine e(r());
        static std::uniform_real_distribution<float> uniform_dist(0, 7);

        int ry = wy - (int)uniform_dist(e);
        if (SNOW_LVL <= ry && ry < world_height)
            voxel_id = SNOW;
        else if (STONE_LVL <= ry && ry < SNOW_LVL)
            voxel_id = STONE;
        else if (DIRT_LVL <= ry && ry < STONE_LVL)
            voxel_id = DIRT;
        else if (GRASS_LVL <= ry && ry < DIRT_LVL)
            voxel_id = GRASS;
        else
            voxel_id = SAND;
    }

    // setting ID
    voxels[get_index(x, y, z)] = voxel_id;

    // place tree
    if (wy < DIRT_LVL) place_tree(voxels, x, y, z, voxel_id);
}

inline int get_chunk_index(int wx, int wy, int wz) {
    int cx = wx / CHUNK_SIZE;
    int cy = wy / CHUNK_SIZE;
    int cz = wz / CHUNK_SIZE;
    if (wx < 0 || cx >= WORLD_W || wy < 0 || cy >= WORLD_H || wz < 0 || cz >= WORLD_D) return -1;

    return cx + WORLD_W * cz + WORLD_AREA * cy;
}

bool is_void(int x, int y, int z, int wx, int wy, int wz,
             const std::array<std::unique_ptr<ChunkMesh::Voxels>, WORLD_VOL>& world_voxels) {
    int chunk_index = get_chunk_index(wx, wy, wz);
    if (chunk_index == -1) return true;

    auto& chunk_voxels = world_voxels[chunk_index];
    int voxel_index = (x + CHUNK_SIZE) % CHUNK_SIZE + (z + CHUNK_SIZE) % CHUNK_SIZE * CHUNK_SIZE +
                      (y + CHUNK_SIZE) % CHUNK_SIZE * CHUNK_AREA;
    if (chunk_voxels->at(voxel_index)) return false;

    return true;
}

std::array<uint8_t, 4> get_ao(int x, int y, int z, int wx, int wy, int wz,
                              const std::array<std::unique_ptr<ChunkMesh::Voxels>, WORLD_VOL>& world_voxels,
                              char plane) {
    uint8_t a, b, c, d, e, f, g, h;
    if (plane == 'Y') {
        a = is_void(x, y, z - 1, wx, wy, wz - 1, world_voxels);
        b = is_void(x - 1, y, z - 1, wx - 1, wy, wz - 1, world_voxels);
        c = is_void(x - 1, y, z, wx - 1, wy, wz, world_voxels);
        d = is_void(x - 1, y, z + 1, wx - 1, wy, wz + 1, world_voxels);
        e = is_void(x, y, z + 1, wx, wy, wz + 1, world_voxels);
        f = is_void(x + 1, y, z + 1, wx + 1, wy, wz + 1, world_voxels);
        g = is_void(x + 1, y, z, wx + 1, wy, wz, world_voxels);
        h = is_void(x + 1, y, z - 1, wx + 1, wy, wz - 1, world_voxels);
    } else if (plane == 'X') {
        a = is_void(x, y, z - 1, wx, wy, wz - 1, world_voxels);
        b = is_void(x, y - 1, z - 1, wx, wy - 1, wz - 1, world_voxels);
        c = is_void(x, y - 1, z, wx, wy - 1, wz, world_voxels);
        d = is_void(x, y - 1, z + 1, wx, wy - 1, wz + 1, world_voxels);
        e = is_void(x, y, z + 1, wx, wy, wz + 1, world_voxels);
        f = is_void(x, y + 1, z + 1, wx, wy + 1, wz + 1, world_voxels);
        g = is_void(x, y + 1, z, wx, wy + 1, wz, world_voxels);
        h = is_void(x, y + 1, z - 1, wx, wy + 1, wz - 1, world_voxels);
    } else {  // Z plane
        a = is_void(x - 1, y, z, wx - 1, wy, wz, world_voxels);
        b = is_void(x - 1, y - 1, z, wx - 1, wy - 1, wz, world_voxels);
        c = is_void(x, y - 1, z, wx, wy - 1, wz, world_voxels);
        d = is_void(x + 1, y - 1, z, wx + 1, wy - 1, wz, world_voxels);
        e = is_void(x + 1, y, z, wx + 1, wy, wz, world_voxels);
        f = is_void(x + 1, y + 1, z, wx + 1, wy + 1, wz, world_voxels);
        g = is_void(x, y + 1, z, wx, wy + 1, wz, world_voxels);
        h = is_void(x - 1, y + 1, z, wx - 1, wy + 1, wz, world_voxels);
    }

    return {static_cast<uint8_t>(a + b + c), static_cast<uint8_t>(g + h + a), static_cast<uint8_t>(e + f + g),
            static_cast<uint8_t>(c + d + e)};
}

inline ChunkMesh::Vertex pack_data(int x, int y, int z, ChunkMesh::Voxels::value_type voxel_id, uint8_t face_id,
                                   uint8_t ao_id, uint8_t flip_id) {
    union {
        struct {
            uint32_t l : 1, a : 2, f : 3, v : 8, z : 6, y : 6, x : 6;
        };
        ChunkMesh::Vertex data;
    } pack_data = {flip_id, ao_id, face_id, voxel_id, (uint8_t)z, (uint8_t)y, (uint8_t)x};

    return pack_data.data;
}

inline void add_data(std::vector<ChunkMesh::Vertex>& vertex_data, std::initializer_list<ChunkMesh::Vertex> vertices) {
    for (auto& vertex : vertices) vertex_data.push_back(vertex);
}
}  // namespace

ChunkMesh::ChunkMesh(Engine& engine, World* world, glm::vec3 pos)
    : Shader("chunk", engine), world(world), position(pos) {
    model = glm::translate(glm::mat4(1), position * (float)CHUNK_SIZE);
    center = (position + 0.5f) * (float)CHUNK_SIZE;

    vert_formats = {vk::Format::eR32Uint};
}

ChunkMesh::~ChunkMesh() {
    // erase them to avoid double free
    uniforms.erase(0);
    uniforms.erase(1);
    uniforms.erase(3);
}

void ChunkMesh::init() {
    // no need to call Shader::init();
    // stolen them from world
    uniforms[0] = world->uniforms[0];
    uniforms[1] = world->uniforms[1];
    uniforms[3] = world->uniforms[3];

    write_vertex(build_mesh());
    write_uniform(2, model);
}

void ChunkMesh::attach() {
    draw_id = vulkan->attachShader(world->vert_shader, world->frag_shader, vertex, vert_formats, uniforms,
                                   world->textures, cull_mode, 0, false);
}

std::unique_ptr<ChunkMesh::Voxels> ChunkMesh::build_voxels() {
    std::unique_ptr<Voxels> voxels = std::make_unique<Voxels>();

    auto chunk_pos = glm::ivec3(position) * CHUNK_SIZE;
    for (int x = 0; x < CHUNK_SIZE; ++x)
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            int wx = x + chunk_pos.x;
            int wz = z + chunk_pos.z;
            int world_height = get_height(glm::vec2(wx, wz));
            int local_height = std::min(world_height - chunk_pos.y, CHUNK_SIZE);

            for (int y = 0; y < local_height; ++y) {
                int wy = y + chunk_pos.y;
                set_voxel_id(*voxels, x, y, z, wx, wy, wz, world_height);
            }
        }

    empty = !std::any_of(voxels->begin(), voxels->end(), [](uint8_t id) { return id != 0; });

    return voxels;
}

std::vector<ChunkMesh::Vertex> ChunkMesh::build_mesh() {
    std::vector<Vertex> mesh;

    // ARRAY_SIZE = CHUNK_VOL * NUM_VOXEL_VERTICES * VERTEX_ATTRS
    // NUM_VOXEL_VERTICES = 3(face) * 2(triagles) * 3(vertices)
    // VERTEX_ATTRS: x, y, z, voxel_id, face_id
    mesh.reserve(CHUNK_VOL * 18);

    for (int x = 0; x < CHUNK_SIZE; ++x)
        for (int y = 0; y < CHUNK_SIZE; ++y)
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                auto voxel_id = (*voxels)[get_index(x, y, z)];
                if (!voxel_id) continue;

                // voxel world position
                auto chunk_pos = position * (float)CHUNK_SIZE;
                int wx = x + (int)chunk_pos.x;
                int wy = y + (int)chunk_pos.y;
                int wz = z + (int)chunk_pos.z;

                Vertex v0, v1, v2, v3;

                // top face
                if (is_void(x, y + 1, z, wx, wy + 1, wz, world->voxels)) {
                    // get AO(ambient occlusion) values
                    auto ao = get_ao(x, y + 1, z, wx, wy + 1, wz, world->voxels, 'Y');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x, y + 1, z, voxel_id, 0, ao[0], flip_id);
                    v1 = pack_data(x + 1, y + 1, z, voxel_id, 0, ao[1], flip_id);
                    v2 = pack_data(x + 1, y + 1, z + 1, voxel_id, 0, ao[2], flip_id);
                    v3 = pack_data(x, y + 1, z + 1, voxel_id, 0, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v1, v0, v3, v1, v3, v2});
                    else
                        add_data(mesh, {v0, v3, v2, v0, v2, v1});
                }

                // bottom face
                if (is_void(x, y - 1, z, wx, wy - 1, wz, world->voxels)) {
                    auto ao = get_ao(x, y - 1, z, wx, wy - 1, wz, world->voxels, 'Y');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x, y, z, voxel_id, 1, ao[0], flip_id);
                    v1 = pack_data(x + 1, y, z, voxel_id, 1, ao[1], flip_id);
                    v2 = pack_data(x + 1, y, z + 1, voxel_id, 1, ao[2], flip_id);
                    v3 = pack_data(x, y, z + 1, voxel_id, 1, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v1, v3, v0, v1, v2, v3});
                    else
                        add_data(mesh, {v0, v2, v3, v0, v1, v2});
                }

                // right face
                if (is_void(x + 1, y, z, wx + 1, wy, wz, world->voxels)) {
                    auto ao = get_ao(x + 1, y, z, wx + 1, wy, wz, world->voxels, 'X');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x + 1, y, z, voxel_id, 2, ao[0], flip_id);
                    v1 = pack_data(x + 1, y + 1, z, voxel_id, 2, ao[1], flip_id);
                    v2 = pack_data(x + 1, y + 1, z + 1, voxel_id, 2, ao[2], flip_id);
                    v3 = pack_data(x + 1, y, z + 1, voxel_id, 2, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v3, v0, v1, v3, v1, v2});
                    else
                        add_data(mesh, {v0, v1, v2, v0, v2, v3});
                }
                // left face
                if (is_void(x - 1, y, z, wx - 1, wy, wz, world->voxels)) {
                    auto ao = get_ao(x - 1, y, z, wx - 1, wy, wz, world->voxels, 'X');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x, y, z, voxel_id, 3, ao[0], flip_id);
                    v1 = pack_data(x, y + 1, z, voxel_id, 3, ao[1], flip_id);
                    v2 = pack_data(x, y + 1, z + 1, voxel_id, 3, ao[2], flip_id);
                    v3 = pack_data(x, y, z + 1, voxel_id, 3, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v3, v1, v0, v3, v2, v1});
                    else
                        add_data(mesh, {v0, v2, v1, v0, v3, v2});
                }
                // back face
                if (is_void(x, y, z - 1, wx, wy, wz - 1, world->voxels)) {
                    auto ao = get_ao(x, y, z - 1, wx, wy, wz - 1, world->voxels, 'Z');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x, y, z, voxel_id, 4, ao[0], flip_id);
                    v1 = pack_data(x, y + 1, z, voxel_id, 4, ao[1], flip_id);
                    v2 = pack_data(x + 1, y + 1, z, voxel_id, 4, ao[2], flip_id);
                    v3 = pack_data(x + 1, y, z, voxel_id, 4, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v3, v0, v1, v3, v1, v2});
                    else
                        add_data(mesh, {v0, v1, v2, v0, v2, v3});
                }
                // front face
                if (is_void(x, y, z + 1, wx, wy, wz + 1, world->voxels)) {
                    auto ao = get_ao(x, y, z + 1, wx, wy, wz + 1, world->voxels, 'Z');
                    bool flip_id = ao[1] + ao[3] > ao[0] + ao[2];

                    v0 = pack_data(x, y, z + 1, voxel_id, 5, ao[0], flip_id);
                    v1 = pack_data(x, y + 1, z + 1, voxel_id, 5, ao[1], flip_id);
                    v2 = pack_data(x + 1, y + 1, z + 1, voxel_id, 5, ao[2], flip_id);
                    v3 = pack_data(x + 1, y, z + 1, voxel_id, 5, ao[3], flip_id);

                    if (flip_id)
                        add_data(mesh, {v3, v1, v0, v3, v2, v1});
                    else
                        add_data(mesh, {v0, v2, v1, v0, v3, v2});
                }
            }

    mesh.shrink_to_fit();
    return mesh;
}

void ChunkMesh::rebuild_mesh() { write_vertex(build_mesh()); }

bool ChunkMesh::is_on_frustum(const Camera& camera) {
    // vector to sphere center
    auto sphere_vec = center - camera.position;

    // outside the NEAR and FAR planes?
    auto sz = glm::dot(sphere_vec, camera.forward);
    if (sz < ZNEAR - CHUNK_SPHERE_RADIUS || sz > ZFAR + CHUNK_SPHERE_RADIUS) return false;

    // outside the TOP and BOTTOM planes?
    auto sy = glm::dot(sphere_vec, camera.up);
    auto dist = camera.frustum.factor_y * CHUNK_SPHERE_RADIUS + sz * camera.frustum.tan_y;
    if (sy < -dist || sy > dist) return false;

    // outside the LEFT and RIGHT planes?
    auto sx = glm::dot(sphere_vec, camera.right);
    dist = camera.frustum.factor_x * CHUNK_SPHERE_RADIUS + sz * camera.frustum.tan_x;
    if (sx < -dist || sx > dist) return false;

    return true;
}
