add_executable(ssr ssr.cc)

target_link_libraries(ssr PRIVATE vkegine)

install(TARGETS ssr DESTINATION .)
