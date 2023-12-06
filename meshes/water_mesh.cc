#include "meshes/water_mesh.h"

#include <vulkan/vulkan_enums.hpp>

#include "engine.h"
#include "settings.h"
#include "shader.h"

WaterMesh::WaterMesh(Engine* engine) : Shader(engine) {
    vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
}

void WaterMesh::init() {
    Shader::init();

    Vertex data[] = {{0, 0, 0, 0, 0}, {1, 0, 1, 1, 1}, {1, 0, 0, 1, 0},
                     {0, 0, 0, 0, 0}, {0, 0, 1, 0, 1}, {1, 0, 1, 1, 1}};
    write_vertex(data);

    write_uniform("water_area", WATER_AREA);
    write_uniform("water_line", WATER_LINE);
    write_texture("u_texture", "water.png");
}
