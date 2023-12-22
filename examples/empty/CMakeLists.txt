add_executable(empty empty.cc)

target_link_libraries(empty PRIVATE vkegine)

install(TARGETS empty DESTINATION .)
