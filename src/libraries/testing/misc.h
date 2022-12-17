#pragma once

#include <concepts>
#include <format>
#include <functional>
#include <iostream>

#include <GLFW/glfw3.h>

// std::source_location would be better
#define expectedValue(V, E) (expectedValueWithLineAndFile(V, E, __FILE__, __LINE__))

template <typename T>
    requires std::integral<T>
constexpr bool expectedValueWithLineAndFile(T value, T expected, const char* file, int line)
{
    if(value != expected)
    {
        std::cerr << std::format(
            "Error: Expected value was: {}. Actual value: {}\nLine {}, {}\n", expected, value, line, file);
        return false;
    }
    return true;
}

void abandonIfFalse(bool b, std::function<void()> cleanup, int code = -1);

struct OpenGLContextForTests
{
    OpenGLContextForTests();
    ~OpenGLContextForTests();
    void cleanup();

  private:
    bool initialized = false;
    GLFWwindow* window;
};

#define GLCONTEXT_CLEANUP(ctx)                                                                               \
    (                                                                                                        \
        [&ctx]()                                                                                             \
        {                                                                                                    \
            ctx.cleanup();                                                                                   \
        })