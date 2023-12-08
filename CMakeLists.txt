cmake_minimum_required(VERSION 3.10)
project(VulkanEngine)


find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(vulkan-engine
    main.cc vulkan.cc engine.cc player.cc shader.cc
    meshes/water_mesh.cc meshes/cloud_mesh.cc
)

target_link_libraries(vulkan-engine PRIVATE glfw)
target_link_libraries(vulkan-engine PRIVATE glm::glm)
target_link_libraries(vulkan-engine PRIVATE glslang::SPIRV glslang::glslang-default-resource-limits)
target_link_libraries(vulkan-engine PRIVATE Vulkan::Headers)
target_link_libraries(vulkan-engine PRIVATE GPUOpen::VulkanMemoryAllocator)
