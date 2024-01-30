add_executable(pbr pbr.cc)

target_link_libraries(pbr PRIVATE vkegine)

install(TARGETS pbr DESTINATION .)
