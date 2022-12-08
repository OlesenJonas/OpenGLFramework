add_subdirectory(thirdparty)

add_subdirectory(intern)

# otherwise glad.h complains that it already has been included
target_compile_definitions(glfw INTERFACE "-DGLFW_INCLUDE_NONE" )

# create list of all libraries that executables will link to
set(LIBS OpenGL::GL glfw glm::glm glad ImGui stb intern PARENT_SCOPE)