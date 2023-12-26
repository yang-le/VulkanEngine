#pragma once

#include "shader.h"

class Scene {
   public:
    virtual ~Scene() = default;

    virtual void init();
    virtual void update() {
        for (auto& mesh : meshes) mesh->update();
    }
    virtual void draw(uint32_t currentBuffer) {
        for (auto& mesh : meshes) mesh->draw(currentBuffer);
    }

    void add_mesh(std::unique_ptr<IShader> shader) { meshes.push_back(std::move(shader)); }

   private:
    virtual void load() {}
    virtual void attach() {}

   private:
    std::vector<std::unique_ptr<IShader>> meshes;
};
