#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "gltf.h"

#include <iostream>

namespace {
vk::SamplerAddressMode getSamplerMode(int wrapMode) {
    switch (wrapMode) {
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return vk::SamplerAddressMode::eClampToEdge;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            [[fallthrough]];
        default:
            return vk::SamplerAddressMode::eRepeat;
    }
}

vk::Filter getFilterMode(int filterMode) {
    switch (filterMode) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            [[fallthrough]];
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            [[fallthrough]];
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return vk::Filter::eNearest;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            [[fallthrough]];
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            [[fallthrough]];
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            [[fallthrough]];
        default:
            return vk::Filter::eLinear;
    }
}

vk::PrimitiveTopology getPrimitveMode(int mode) {
    switch (mode) {
        case TINYGLTF_MODE_POINTS:
            return vk::PrimitiveTopology::ePointList;
        case TINYGLTF_MODE_LINE:
            return vk::PrimitiveTopology::eLineList;
        case TINYGLTF_MODE_LINE_LOOP:
            [[fallthrough]];  // ???
        case TINYGLTF_MODE_LINE_STRIP:
            return vk::PrimitiveTopology::eLineStrip;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
            return vk::PrimitiveTopology::eTriangleStrip;
        case TINYGLTF_MODE_TRIANGLE_FAN:
            return vk::PrimitiveTopology::eTriangleFan;
        case TINYGLTF_MODE_TRIANGLES:
            [[fallthrough]];
        default:
            return vk::PrimitiveTopology::eTriangleList;
    }
}

vk::IndexType getIndexType(int componentType) {
    assert(componentType != TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(componentType != TINYGLTF_COMPONENT_TYPE_DOUBLE);

    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_INT:
            [[fallthrough]];
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return vk::IndexType::eUint32;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            [[fallthrough]];
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return vk::IndexType::eUint16;
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            [[fallthrough]];
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return vk::IndexType::eUint8EXT;
        default:
            return vk::IndexType::eNoneKHR;
    }
}
}  // namespace

namespace gltf {

Model::Model(Vulkan* vulkan, const std::string& filename) : vulkan(vulkan) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool binary = false;
    auto extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos) binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");

    bool fileLoaded = binary ? loader.LoadBinaryFromFile(&model, &err, &warn, filename)
                             : loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!err.empty()) std::cerr << err;
    if (!warn.empty()) std::cerr << warn;
    if (!fileLoaded) throw std::runtime_error("Fail to load: " + filename);

    // maybe lazy load in the future
    loadTextures();
    loadBufferViews();
}

void Model::load(Node* parent, const tinygltf::Node& node) {
    Node newNode{};
    newNode.parent = parent;
    newNode.matrix = glm::mat4(1.0f);

    // Generate local node matrix
    if (node.translation.size() == 3) newNode.translation = glm::make_vec3(node.translation.data());
    if (node.rotation.size() == 4) newNode.rotation = glm::make_quat(node.rotation.data());
    if (node.scale.size() == 3) newNode.scale = glm::make_vec3(node.scale.data());
    if (node.matrix.size() == 16) newNode.matrix = glm::make_mat4(node.matrix.data());  // is this colunm major?

    // Load Mesh
    if (node.mesh > -1) {
        load(model.meshes[node.mesh]);
        newNode.mesh = &meshes.back();
    }

    if (parent) {
        parent->children.push_back(newNode);
        for (auto i : node.children) load(&parent->children.back(), model.nodes[i]);

    } else {
        nodes.push_back(newNode);
        for (auto i : node.children) load(&nodes.back(), model.nodes[i]);
    }
}

void Model::load(const tinygltf::Mesh& mesh) {
    Mesh newMesh{};
    for (auto& primitive : mesh.primitives) {
        Primitive newPrimitive{};
        newPrimitive.mode = getPrimitveMode(primitive.mode);

        auto indexAccessor = model.accessors[primitive.indices];
        assert(indexAccessor.type == TINYGLTF_TYPE_SCALAR);

        auto indexStride = indexAccessor.ByteStride(model.bufferViews[indexAccessor.bufferView]);

        // correct the buffer stride, is here a suitable place?
        bufferViews[indexAccessor.bufferView].stride = indexStride;

        newPrimitive.index = bufferViews[indexAccessor.bufferView].buffer;
        newPrimitive.indexOffset = indexAccessor.byteOffset;
        newPrimitive.indexCount = indexAccessor.count;
        newPrimitive.indexType = getIndexType(indexAccessor.componentType);

        for (auto& attribute : primitive.attributes) {
            auto accesor = model.accessors[attribute.second];
            auto stride = accesor.ByteStride(model.bufferViews[accesor.bufferView]);
            bufferViews[accesor.bufferView].stride = stride;

            newPrimitive.vertex.push_back(bufferViews[accesor.bufferView].buffer);
            newPrimitive.vertexOffset.push_back(accesor.byteOffset);
            newPrimitive.vertexStrides.push_back(stride);
        }

        // record and get drawId

        newMesh.primitives.push_back(newPrimitive);
    }
    meshes.push_back(newMesh);
}

void Model::loadTextures() {
    for (auto& tex : model.textures) {
        tinygltf::Image image = model.images[tex.source];
        Vulkan::Texture texture;
        if (tex.sampler == -1)
            texture = vulkan->createTexture({(uint32_t)image.width, (uint32_t)image.height}, image.image.data());
        else {
            auto sampler = model.samplers[tex.sampler];
            texture = vulkan->createTexture({(uint32_t)image.width, (uint32_t)image.height}, image.image.data(), 1,
                                            true, getFilterMode(sampler.magFilter), getFilterMode(sampler.minFilter),
                                            getSamplerMode(sampler.wrapS), getSamplerMode(sampler.wrapT),
                                            getSamplerMode(sampler.wrapT));
        }
        textures.push_back(texture);
    }
}
}  // namespace gltf
