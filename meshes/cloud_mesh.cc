#include "meshes/cloud_mesh.h"

#include <glm/gtc/noise.hpp>

#include "engine.h"

CloudMesh::CloudMesh(Engine* engine) : Shader("clouds", engine) {
    vert_formats = {vk::Format::eR32G32B32Sfloat};
    cull_mode = vk::CullModeFlagBits::eNone;
}

void CloudMesh::init() {
    Shader::init();

    write_vertex(build_mesh());

    write_uniform(2, 0.0f);
    write_uniform(3, BG_COLOR, vk::ShaderStageFlagBits::eFragment);
}

void CloudMesh::update() {
    Shader::update();

    write_uniform(2, engine->t);
}

void CloudMesh::gen_clouds() {
    for (int x = 0; x < WORLD_W * CHUNK_SIZE; ++x)
        for (int z = 0; z < WORLD_D * CHUNK_SIZE; ++z) {
            if (glm::simplex(glm::vec2(0.13 * x, 0.13 * z)) < 0.2) continue;
            cloud_data.set(x + WORLD_W * CHUNK_SIZE * z);
        }
}

std::vector<CloudMesh::Vertex> CloudMesh::build_mesh() {
    std::vector<Vertex> mesh;
    mesh.reserve(WORLD_AREA * CHUNK_AREA * 6);

    gen_clouds();

    constexpr int width = WORLD_W * CHUNK_SIZE;
    constexpr int depth = WORLD_D * CHUNK_SIZE;
    constexpr int y = CLOUD_HEIGHT;
    std::set<int> visited;

    for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x) {
            int idx = x + width * z;
            if (!cloud_data[idx] || visited.find(idx) != visited.end()) continue;

            // find number of continuous qauds along x
            int x_cont = 1;
            idx = x + x_cont + width * z;
            while (x + x_cont < width && cloud_data[idx] && visited.find(idx) == visited.end()) {
                ++x_cont;
                idx = x + x_cont + width * z;
            }

            // find number of continuous quads along z for each x
            int z_cont_min = std::numeric_limits<int>::max();
            for (int ix = 0; ix < x_cont; ++ix) {
                int z_cont = 1;
                idx = x + ix + width * (z + z_cont);
                while (z + z_cont < depth && cloud_data[idx] && visited.find(idx) == visited.end()) {
                    ++z_cont;
                    idx = x + ix + width * (z + z_cont);
                }

                // find min cont z to form a large qaud
                if (z_cont < z_cont_min) z_cont_min = z_cont;
            }

            int z_cont = (z_cont_min < std::numeric_limits<int>::max()) ? z_cont_min : 1;

            // mark all unit quads of the large quad as visited
            for (int ix = 0; ix < x_cont; ++ix)
                for (int iz = 0; iz < z_cont; ++iz) visited.insert(x + ix + width * (z + iz));

            Vertex v[] = {{(float)x, y, (float)z},
                          {(float)x + x_cont, y, (float)z + z_cont},
                          {(float)x + x_cont, y, (float)z},
                          {(float)x, y, (float)z + z_cont}};

            // scale
            for (auto& pos : v) {
                pos.x = (pos.x - CENTER_XZ) * CLOUD_SCALE + CENTER_XZ;
                pos.z = (pos.z - CENTER_XZ) * CLOUD_SCALE + CENTER_XZ;
            }

            for (const auto& vertex : {v[0], v[1], v[2], v[0], v[3], v[1]}) {
                mesh.push_back(vertex);
            }
        }

    mesh.shrink_to_fit();
    return mesh;
}
