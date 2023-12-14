#include "scene.h"

#include "engine.h"

Scene::Scene(Engine* engine) : engine(engine), vulkan(&engine->vulkan) { world = std::make_unique<World>(engine); }

void Scene::init() {
    world->init();
    for (auto& mesh : meshes) mesh->init();

    glslang::InitializeProcess();
    world->load();
    for (auto& mesh : meshes) mesh->load();
    glslang::FinalizeProcess();

    world->attach();
    for (auto& mesh : meshes) mesh->attach();
}

void Scene::update() {
    world->update();
    for (auto& mesh : meshes) mesh->update();
}

void Scene::draw() {
    world->draw();
    for (auto& mesh : meshes) mesh->draw();
}

void Scene::add_mesh(std::unique_ptr<Shader> shader) { meshes.push_back(std::move(shader)); }
