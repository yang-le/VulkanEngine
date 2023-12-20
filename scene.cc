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
