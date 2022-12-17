cmake_minimum_required(VERSION 3.2)
include(DefaultLibrary)
target_link_libraries(testing PRIVATE glfw)
target_link_libraries(testing PRIVATE glad)