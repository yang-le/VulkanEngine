#pragma once

#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine.h"
#include "shader.h"

namespace gltf {
struct Primitive {
    vk::Buffer index;
    vk::DeviceSize indexOffset;
    size_t indexCount;
    vk::IndexType indexType;
    std::vector<vk::Buffer> vertex;
    std::vector<vk::DeviceSize> vertexOffset;
    std::vector<uint32_t> vertexStrides;
    vk::PrimitiveTopology mode;
    uint32_t drawId;
};

struct Mesh {
    std::vector<Primitive> primitives;
};

struct Node {
    Node* parent;                // need to track the parent to get final matrix
    std::vector<Node> children;  // need to draw
    Mesh* mesh;
    glm::mat4 matrix;
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::quat rotation{};
    glm::mat4 localMatrix() {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) *
               matrix;
    }
    glm::mat4 getMatrix() {
        glm::mat4 m = localMatrix();
        for (Node* p = parent; p; p = p->parent) m *= p->localMatrix();
        return m;
    }
};

struct Model {
    Model(Vulkan* vulkan, const std::string& filename);
    ~Model() {
        for (auto& texture : textures) vulkan->destroyTexture(texture);
        for (auto& buffer : bufferViews) vulkan->destroyGltfBuffer(buffer);
    }
    void load(size_t i = -1) {
        if (i == -1) i = model.defaultScene > -1 ? model.defaultScene : 0;
        for (auto j : model.scenes[i].nodes) load(nullptr, model.nodes[j]);
    }
    void load(Node* parent, const tinygltf::Node& node);
    void load(const tinygltf::Mesh& mesh);

    void draw(const Primitive& primitive) {
        vulkan->drawIndex(primitive.drawId, primitive.index, primitive.indexOffset, primitive.indexType,
                          (uint32_t)primitive.indexCount, primitive.vertex, primitive.vertexOffset);
    }
    void draw(const Mesh& mesh) {
        for (auto& primitive : mesh.primitives) draw(primitive);
    }
    void draw(const Node& node) {
        if (node.mesh) draw(*node.mesh);
        for (auto& child : node.children) draw(child);
    }
    void draw(size_t i = -1) {
        if (i == -1) i = model.defaultScene > -1 ? model.defaultScene : 0;
        for (auto j : model.scenes[i].nodes) draw(nodes[j]);
    }

    void attachShader(Primitive& primitive, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Vulkan::Buffer>& uniforms,
                      const std::map<int, Vulkan::Texture>& textures, uint32_t subpass,
                      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack) {
        primitive.drawId =
            vulkan->attachShader(vertexShaderModule, fragmentShaderModule, primitive.vertexStrides, vertexFormats,
                                 uniforms, textures, primitive.mode, subpass, cullMode, false);
    }

    void attachShader(Mesh& mesh, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Vulkan::Buffer>& uniforms,
                      const std::map<int, Vulkan::Texture>& textures, uint32_t subpass,
                      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack) {
        for (auto& primitive : mesh.primitives)
            attachShader(primitive, vertexShaderModule, fragmentShaderModule, vertexFormats, uniforms, textures,
                         subpass, cullMode);
    }

    void attachShader(Node& node, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Vulkan::Buffer>& uniforms,
                      const std::map<int, Vulkan::Texture>& textures, uint32_t subpass,
                      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack) {
        if (node.mesh)
            attachShader(*node.mesh, vertexShaderModule, fragmentShaderModule, vertexFormats, uniforms, textures,
                         subpass, cullMode);
        for (auto& child : node.children)
            attachShader(child, vertexShaderModule, fragmentShaderModule, vertexFormats, uniforms, textures, subpass,
                         cullMode);
    }

    void attachShader(vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Vulkan::Buffer>& uniforms,
                      const std::map<int, Vulkan::Texture>& textures, uint32_t subpass, size_t i = -1,
                      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack) {
        if (i == -1) i = model.defaultScene > -1 ? model.defaultScene : 0;
        for (auto j : model.scenes[i].nodes)
            attachShader(nodes[j], vertexShaderModule, fragmentShaderModule, vertexFormats, uniforms, textures, subpass,
                         cullMode);

        vulkan->destroyShaderModule(fragmentShaderModule);
        vulkan->destroyShaderModule(vertexShaderModule);
    }

    void loadTextures();
    void loadBufferViews() {
        for (auto& bufferView : model.bufferViews)
            bufferViews.push_back(vulkan->createGltfBuffer(
                model.buffers[bufferView.buffer].data.data() + bufferView.byteOffset, bufferView.byteLength));
    }

    tinygltf::Model model;

    Vulkan* vulkan;
    std::vector<Vulkan::Texture> textures;
    std::vector<Vulkan::Buffer> bufferViews;
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;  // for draw
};

struct Shader : ::Shader {
    Shader(const std::string& gltf_file, const std::string& name, Engine& engine)
        : ::Shader(name, engine), model(&engine.vulkan, "assets/" + gltf_file) {
        model.load();
    }

    virtual void attach(uint32_t subpass = 0) override {
        model.attachShader(vert_shader, frag_shader, vert_formats, uniforms, textures, subpass);
    }

    virtual void draw() override { model.draw(); }

    Model model;
};
}  // namespace gltf
