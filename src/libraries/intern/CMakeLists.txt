cmake_minimum_required(VERSION 3.2)
include(DefaultLibrary)
#need to link as public, so tests linking against "intern" also get them
target_link_libraries(intern PUBLIC glfw)
target_link_libraries(intern PUBLIC glm::glm)
target_link_libraries(intern PUBLIC glad)
target_link_libraries(intern PUBLIC ImGui)
target_link_libraries(intern PUBLIC stb)