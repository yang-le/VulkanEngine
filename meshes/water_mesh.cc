#include "meshes/water_mesh.h"

#include "engine.h"

WaterMesh::WaterMesh(Engine* engine) : Shader("water", engine) {
    vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
    cull_mode = vk::CullModeFlagBits::eNone;
}

void WaterMesh::init() {
    Shader::init();

    Vertex data[] = {{0, 0, 0, 0, 0}, {1, 0, 1, 1, 1}, {1, 0, 0, 1, 0},
                     {0, 0, 0, 0, 0}, {0, 0, 1, 0, 1}, {1, 0, 1, 1, 1}};
    write_vertex(data);

    write_texture(2, "water.png");
}
