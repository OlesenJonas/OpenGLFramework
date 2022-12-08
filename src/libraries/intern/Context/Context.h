#pragma once

#include <GLFW/glfw3.h>

#include <intern/Camera/Camera.h>
#include <intern/InputManager/InputManager.h>

// very simple context struct to pass "global" objects around
class Context
{
  public:
    Context() = default;

    GLFWwindow* getWindow();
    void setWindow(GLFWwindow* window);
    InputManager* getInputManager();
    void setInputManager(InputManager* inputManager);
    Camera* getCamera();
    void setCamera(Camera* camera);

  private:
    // todo: window abstraction object that stores width/height etc
    GLFWwindow* window;
    InputManager* inputManager;
    Camera* camera;
};