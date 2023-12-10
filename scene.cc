#include "scene.h"

#include "engine.h"

Scene::Scene(Engine* egnine) : engie(egnine), vulkan(&engie->vulkan) { world = std::make_unique<World>(engie); }

void Scene::init() {
    world->init();
    for (auto& mesh : meshes) mesh->init();

    glslang::InitializeProcess();
    world->load();
    for (auto& mesh : meshes) mesh->load();
    glslang::FinalizeProcess();

    for (auto& mesh : meshes)
        vulkan->attachShader(mesh->vert_shader, mesh->frag_shader, mesh->vertex, mesh->vert_formats, mesh->uniforms,
                             mesh->textures, mesh->cull_mode);
    for (auto& mesh : world->chunks)
        vulkan->attachShader(mesh->vert_shader, mesh->frag_shader, mesh->vertex, mesh->vert_formats, mesh->uniforms,
                             mesh->textures, mesh->cull_mode);
}

void Scene::update() {
    world->update();
    for (auto& mesh : meshes) mesh->update();
}

void Scene::add_mesh(std::unique_ptr<Shader> shader) { meshes.push_back(std::move(shader)); }
