add_executable(shadow shadow.cc)

target_link_libraries(shadow PRIVATE vkegine)

install(TARGETS shadow DESTINATION .)
