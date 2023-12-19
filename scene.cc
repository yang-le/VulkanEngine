#include "scene.h"

void Scene::init() {
    for (auto& mesh : meshes) mesh->init();

    glslang::InitializeProcess();
    load();
    for (auto& mesh : meshes) mesh->load();
    glslang::FinalizeProcess();

    attach();
    for (auto& mesh : meshes) mesh->attach();
}

void Scene::update() {
    for (auto& mesh : meshes) mesh->update();
}

void Scene::draw() {
    for (auto& mesh : meshes) mesh->draw();
}

void Scene::add_mesh(std::unique_ptr<Shader> shader) { meshes.push_back(std::move(shader)); }
