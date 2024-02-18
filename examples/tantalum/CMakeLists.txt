add_executable(tantalum tantalum.cc)

target_link_libraries(tantalum PRIVATE vkegine)

install(TARGETS tantalum DESTINATION .)
