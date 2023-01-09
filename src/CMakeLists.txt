include(FindOpenGL)

# get libraries from vcpk
find_package(glfw3 CONFIG REQUIRED)
target_compile_definitions(glfw INTERFACE "-DGLFW_INCLUDE_NONE" )
find_package(glm CONFIG REQUIRED)
# theres this Cmakes findPNG module that also works with vcpkg
# https://cmake.org/cmake/help/latest/module/FindPNG.html
# include(FindPNG)
# but the vcpkg output says just to use
find_package(PNG REQUIRED)

# create libraries from local files
add_subdirectory("libraries/")

# create executables
add_subdirectory("executables/")

# create tests
add_subdirectory("tests/")