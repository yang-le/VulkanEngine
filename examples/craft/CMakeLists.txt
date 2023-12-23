include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(vkcraft
    craft.cc world.cc
    meshes/chunk_mesh.cc meshes/voxel_marker.cc
    meshes/water_mesh.cc meshes/cloud_mesh.cc
)

target_link_libraries(vkcraft PRIVATE vkegine)

install(TARGETS vkcraft DESTINATION .)
