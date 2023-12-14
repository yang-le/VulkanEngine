#include "voxel_marker.h"

#include "engine.h"

VoxelMarkerMesh::VoxelMarkerMesh(Engine* engine) : Shader("voxel_marker", engine) {
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

    write_vertex(hstack(get_data(vertices, indices), get_data(tex_coord_vertices, tex_coord_indices)));

    write_uniform(2, glm::mat4(1));
    write_uniform(3, 0);
    write_texture(4, "frame.png");
}

void VoxelMarkerMesh::update() {}
