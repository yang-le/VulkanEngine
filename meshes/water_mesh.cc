#include "meshes/water_mesh.h"

#include "engine.h"

WaterMesh::WaterMesh(Engine& engine) : Shader("water", engine) {
    vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
    cull_mode = vk::CullModeFlagBits::eNone;
}

void WaterMesh::init() {
    Shader::init();

    std::array<Vertex, 6> datas = {Vertex{{0, 0, 0}, {0, 0}}, {{1, 0, 1}, {1, 1}}, {{1, 0, 0}, {1, 0}},
                                   {{0, 0, 0}, {0, 0}},       {{0, 0, 1}, {0, 1}}, {{1, 0, 1}, {1, 1}}};

    for (auto& data : datas) {
        data.pos.x = (data.pos.x - 0.33) * WATER_AREA;
        data.pos.z = (data.pos.z - 0.33) * WATER_AREA;
        data.tex *= WATER_AREA;
    }

    write_vertex(datas);

    write_texture(2, "water.png");
}
