#include "shader.h"
#include "engine.h"


namespace {
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open a file!");
        }

        auto fileSize = file.tellg();
        std::vector<char> fileBuffer(1 + fileSize);

        file.seekg(std::ios::beg);
        file.read(fileBuffer.data(), fileSize);
        file.close();

        return fileBuffer;
    }
}

Shader::Shader(Engine* engine)
    : engine(engine), vulkan(&engine->vulkan), player(&engine->player)
{
}

void Shader::load(const std::string& shader_name)
{
    glslang::InitializeProcess();
    vert_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eVertex, readFile("shaders/" + shader_name + ".vert").data());
    frag_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eFragment, readFile("shaders/" + shader_name + ".frag").data());
    glslang::FinalizeProcess();
}
