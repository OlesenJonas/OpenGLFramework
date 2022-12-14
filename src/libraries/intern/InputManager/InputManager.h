#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <iostream>

class Context;

class InputManager
{
  public:
    explicit InputManager(Context& ctx);

    void update();

    void resetTime(int64_t frameCount = 0, double simulationTime = 0.0);
    void disableFixedTimestep();
    void enableFixedTimestep(double timestep);
    void setupCallbacks(
        GLFWkeyfun keyCallback = nullptr, GLFWmousebuttonfun mousebuttonCallback = nullptr,
        GLFWscrollfun scrollCallback = nullptr, GLFWframebuffersizefun resizeCallback = nullptr);

    inline glm::vec2 getMouseDelta() const
    {
        return mouseDelta;
    }

    inline float getRealDeltaTime() const
    {
        return static_cast<float>(deltaTime);
    }
    inline float getSimulationDeltaTime() const
    {
        return static_cast<float>(useFixedTimestep ? fixedDeltaTime : deltaTime);
    }
    inline double getSimulationTime() const
    {
        return simulationTime;
    };
    inline int64_t getFrameCount() const
    {
        return frameCount;
    };

    static void defaultMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void defaultKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void defaultScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void defaultResizeCallback(GLFWwindow* window, int width, int height);

  private:
    Context& ctx;

    /* Track simulation and real time seperately so that input etc still function correctly */
    int64_t frameCount = -1;
    double realTime = 0.0;
    double simulationTime = 0.0;
    double fixedDeltaTime = 0.0;
    double deltaTime = 0.0;
    bool useFixedTimestep = false;

    double mouseX;
    double mouseY;
    double oldMouseX;
    double oldMouseY;
    glm::vec2 mouseDelta{0.0f, 0.0f};
};
