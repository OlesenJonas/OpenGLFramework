#include "misc.h"

#include <glad/glad/glad.h>

#include <intern/Window/Window.h>
#include <intern/misc/OpenGLErrorHandler.h>

void abandonIfFalse(bool b, std::function<void()> cleanup, int code)
{
    if(!b)
    {
        cleanup();
        exit(code);
    }
}

OpenGLContextForTests::OpenGLContextForTests()
{
    // todo: catch errors!
    glfwInit();
    window = initAndCreateGLFWWindow(500, 500, "CBT_Test", {{GLFW_VISIBLE, GLFW_FALSE}});
    if(window == nullptr)
    {
        std::cerr << "Failed to create window" << std::endl;
    }
    if(gladLoadGL() == 0)
    {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
    }
    setupOpenGLMessageCallback();
    initialized = true;
}

OpenGLContextForTests::~OpenGLContextForTests()
{
    if(initialized)
    {
        cleanup();
    }
}

void OpenGLContextForTests::cleanup()
{
    std::cerr << "Destroying OpenGL context and GLFW window" << std::endl;
    glfwSetWindowShouldClose(window, true);
    glfwDestroyWindow(window);
    glfwTerminate();
    initialized = false;
};