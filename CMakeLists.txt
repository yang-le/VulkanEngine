cmake_minimum_required(VERSION 3.10)
project(VulkanEngine)

set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_executable(vulkan-engine main.cc vulkan.cc)
target_link_libraries(vulkan-engine PRIVATE glfw)
target_link_libraries(vulkan-engine PRIVATE glm::glm)
target_link_libraries(vulkan-engine PRIVATE glslang::OSDependent glslang::glslang glslang::MachineIndependent glslang::GenericCodeGen glslang::glslang-default-resource-limits glslang::OGLCompiler glslang::SPVRemapper glslang::SPIRV glslang::HLSL)
target_link_libraries(vulkan-engine PRIVATE Vulkan::Headers)
target_link_libraries(vulkan-engine PRIVATE GPUOpen::VulkanMemoryAllocator)
