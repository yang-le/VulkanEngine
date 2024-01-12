add_executable(prt prt.cc)

target_link_libraries(prt PRIVATE vkegine)

install(TARGETS prt DESTINATION .)
