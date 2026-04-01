add_executable(elfdump elfdump.cpp)
target_link_libraries(elfdump PRIVATE elfio-modern)

add_executable(write_obj write_obj.cpp)
target_link_libraries(write_obj PRIVATE elfio-modern)
