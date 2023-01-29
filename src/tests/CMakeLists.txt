set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/out/debug_tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/out/debug_tests)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out/release_tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/out/release_tests)

# todo:
# could replace these hardcoded entries
# with a structure similar to the executables folder

# Define the executable
add_executable(CBT_CPU_test CBT_CPU.cpp)
# link needed libraries
target_link_libraries(CBT_CPU_test intern testing)
# Define the test
add_test(NAME "run_CBT_CPU_test" COMMAND $<TARGET_FILE:CBT_CPU_test>)

# Define the executable
add_executable(CBT_GPU_test CBT_GPU.cpp)
# link needed libraries
target_link_libraries(CBT_GPU_test intern testing)
# Define the test
add_test(NAME "run_CBT_GPU_test" COMMAND $<TARGET_FILE:CBT_GPU_test>)

# Define the executable
add_executable(TexFormat_test TextureFormats.cpp)
# link needed libraries
target_link_libraries(TexFormat_test intern testing)
# Define the test
add_test(NAME "run_texFormat_test" COMMAND $<TARGET_FILE:TexFormat_test>)