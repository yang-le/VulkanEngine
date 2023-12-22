add_executable(gltf gltf.cc)

target_link_libraries(gltf PRIVATE vkegine)

install(TARGETS gltf DESTINATION .)
