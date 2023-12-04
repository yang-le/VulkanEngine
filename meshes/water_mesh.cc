#include "meshes/water_mesh.h"

#include "engine.h"
#include "settings.h"
#include "shader.h"
#include <vulkan/vulkan_enums.hpp>


WaterMesh::WaterMesh(Engine* engine)
    : Shader(engine)
{
    vert_format = {
        {vk::Format::eR8G8Uint, 0},
        {vk::Format::eR8G8B8Uint, 2}
    };
}

void WaterMesh::init() {
    Shader::init();

    Vertex data[] = {
        {0, 0, 0, 0, 0},
        {1, 1, 1, 0, 1},
        {1, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 1, 0, 0, 1},
        {1, 1, 1, 0, 1}
    };
    write(data);

    write("water_area", WATER_AREA);
    write("water_line", WATER_LINE);
    write("u_texture", "water.png");
}
