#pragma once

#include <GLFW/glfw3.h>

#include <intern/Camera/Camera.h>
#include <intern/InputManager/InputManager.h>
#include <intern/Scene/Scene.h>

// very simple context struct to pass "global" objects around
class Context
{
  public:
    static Context* globalContext;

    Context() = default;

    GLFWwindow* getWindow();
    void setWindow(GLFWwindow* window);
    InputManager* getInputManager();
    void setInputManager(InputManager* inputManager);
    Camera* getCamera();
    void setCamera(Camera* camera);
    void* getUserPointer();
    void setUserPointer(void* ptr);

    int outputWidth = 0;
    int outputHeight = 0;
    int internalWidth = 0;
    int internalHeight = 0;
    GLFWwindow* window;
    InputManager* inputManager;
    Camera* camera;
    Scene* scene;

    void* userPointer;
};