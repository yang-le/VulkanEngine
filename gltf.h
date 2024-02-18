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

    glm::vec4 baseColorFactor;
    Vulkan::Texture* baseColorTexture = nullptr;
    Vulkan::Texture* normalTexture = nullptr;

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
    glm::mat4 localMatrix() const {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) *
               matrix;
    }
    glm::mat4 getMatrix() const {
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

struct PrimitiveShader : ::Shader {
    PrimitiveShader(Primitive& primitive, const std::string& name, Engine& engine)
        : primitive(primitive), ::Shader(name, engine) {}

    virtual void attach(uint32_t subpass = 0) override {
        draw_id = vulkan->attachShader(vert_shader, frag_shader, primitive.vertexStrides, vert_formats, uniforms,
                                       textures, primitive.mode, subpass, cull_mode);
    }

    virtual void draw() override {
        vulkan->drawIndexed(draw_id, primitive.index, primitive.indexOffset, primitive.indexType,
                            (uint32_t)primitive.indexCount, primitive.vertex, primitive.vertexOffset);
    }

    Primitive& primitive;
};

struct Shader : ::Shader {
    Shader(const std::string& gltf_file, const std::string& name, Engine& engine)
        : ::Shader(name, engine), model(&engine.vulkan, gltf_file) {
        model.load();
    }

    void attachShader(Node& node, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
                      const std::vector<vk::Format>& vertexFormats, const std::map<int, Vulkan::Buffer>& uniforms,
                      const std::map<int, Vulkan::Texture>& textures, uint32_t subpass, vk::CullModeFlags cullMode) {
        if (node.mesh)
            for (auto& primitive : node.mesh->primitives)
                primitive.drawId =
                    vulkan->attachShader(vertexShaderModule, fragmentShaderModule, primitive.vertexStrides,
                                         vertexFormats, uniforms, textures, primitive.mode, subpass, cullMode, false);
        for (auto& child : node.children)
            attachShader(child, vertexShaderModule, fragmentShaderModule, vertexFormats, uniforms, textures, subpass,
                         cullMode);
    }

    void drawNode(const Node& node) {
        if (node.mesh)
            for (auto& primitive : node.mesh->primitives)
                vulkan->drawIndexed(primitive.drawId, primitive.index, primitive.indexOffset, primitive.indexType,
                                    (uint32_t)primitive.indexCount, primitive.vertex, primitive.vertexOffset);
        for (auto& child : node.children) drawNode(child);
    }

    virtual void attach(uint32_t subpass = 0) override {
        int i = model.model.defaultScene > -1 ? model.model.defaultScene : 0;
        for (auto j : model.model.scenes[i].nodes)
            attachShader(model.nodes[j], vert_shader, frag_shader, vert_formats, uniforms, textures, subpass,
                         cull_mode);

        vulkan->destroyShaderModule(vert_shader);
        vulkan->destroyShaderModule(frag_shader);
    }

    virtual void draw() override {
        int i = model.model.defaultScene > -1 ? model.model.defaultScene : 0;
        for (auto j : model.model.scenes[i].nodes) drawNode(model.nodes[j]);
    }

    Model model;
};
}  // namespace gltf
