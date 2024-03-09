cmake_minimum_required(VERSION 3.10)
project(VulkanEngine)

option(BUILD_SHARED_LIBS "Build Shared Libs" ON)

set (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ${BUILD_SHARED_LIBS})

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
if (OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS})
endif (OpenMP_CXX_FOUND)
find_package(imgui CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

add_library(vkegine vulkan.cc engine.cc player.cc shader.cc scene.cc gltf.cc audio.cc)

target_link_libraries(vkegine PUBLIC glfw)
target_link_libraries(vkegine PUBLIC glm::glm)
target_link_libraries(vkegine PUBLIC glslang::SPIRV glslang::glslang-default-resource-limits)
target_link_libraries(vkegine PUBLIC Vulkan::Headers)
target_link_libraries(vkegine PUBLIC GPUOpen::VulkanMemoryAllocator)
target_link_libraries(vkegine PUBLIC imgui::imgui)
target_link_libraries(vkegine PUBLIC OpenAL::OpenAL)

add_subdirectory(examples)

if (${BUILD_SHARED_LIBS})
    install(TARGETS vkegine DESTINATION .)
endif (${BUILD_SHARED_LIBS})
install(FILES ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/glfw3.dll DESTINATION .)
install(DIRECTORY assets DESTINATION .)
install(DIRECTORY shaders DESTINATION .)

set(CPACK_PACKAGE_NAME "VulkanEngine")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "ZIP")
include(CPack)
