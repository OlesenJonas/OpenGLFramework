#include <glad/glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/transform.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_opengl3.h>

#include <intern/Camera/Camera.h>
#include <intern/Context/Context.h>
#include <intern/Framebuffer/Framebuffer.h>
#include <intern/InputManager/InputManager.h>
#include <intern/Mesh/Cube.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/Misc/ImGuiExtensions.h>
#include <intern/Misc/OpenGLErrorHandler.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Window/Window.h>

int main()
{
    Context ctx{};

    //----------------------- INIT WINDOW

    int WIDTH = 1200;
    int HEIGHT = 800;

    GLFWwindow* window = initAndCreateGLFWWindow(
        WIDTH, HEIGHT, "Texured Cube and Framebuffer example", {{GLFW_MAXIMIZED, GLFW_TRUE}});

    ctx.setWindow(window);
    // disable VSYNC
    glfwSwapInterval(0);

    // In case window was set to start maximized, retrieve size for framebuffer here
    glfwGetWindowSize(window, &WIDTH, &HEIGHT);

    //----------------------- INIT OpenGL
    // init OpenGL context
    if(gladLoadGL() == 0)
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }
#ifndef NDEBUG
    setupOpenGLMessageCallback();
#endif
    glClearColor(0.3f, 0.7f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    // glEnable(GL_FRAMEBUFFER_SRGB);

    //----------------------- INIT IMGUI & Input

    InputManager input(ctx);
    ctx.setInputManager(&input);
    input.setupCallbacks();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    // Loading a custom font
    // io.Fonts->AddFontFromFileTTF("C:/WINDOWS/Fonts/verdana.ttf", FONT_SIZE, NULL, NULL);
    ImGui::StyleColorsDark();
    // platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    //----------------------- INIT REST

    FullscreenTri fullScreenTri;
    ShaderProgram postProcessShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/General/hdrTonemapSimple.frag"}};

    Framebuffer internalFBO{WIDTH, HEIGHT, {GL_RGBA16F}, true};

    // todo: not sure if I want the ctx.setXXX() functions to be part of the constructors
    //       any occasions where it would be undesirable?
    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT)};
    ctx.setCamera(&cam);

    Cube cube{1.0f};
    ShaderProgram simpleShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/simpleTexture.vert", SHADERS_PATH "/General/simpleTexture.frag"}};

    const Texture gridTexture{MISC_PATH "/GridTexture.png", true};

    //----------------------- RENDERLOOP

    // reset time to 0 before renderloop starts
    glfwSetTime(0.0);
    input.resetTime();

    while(glfwWindowShouldClose(window) == 0)
    {
        ImGui::Extensions::FrameStart();

        // input needs to be updated (checks for pressed buttons etc.)
        input.update();
        // dont update camera if UI is using user inputs
        if(!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard)
        {
            cam.update();
        }

        auto currentTime = static_cast<float>(input.getSimulationTime());

        internalFBO.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw into internal framebuffer
        simpleShader.useProgram();
        glBindTextureUnit(0, gridTexture.getTextureID());
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::mat4{1.0f}));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        cube.draw();

        // Post Processing (writes internal framebuffer to default framebuffer)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // overwriting full screen anyways, dont need to clear
            glDisable(GL_DEPTH_TEST);
            glBindTextureUnit(0, internalFBO.getColorTextures()[0].getTextureID());
            postProcessShader.useProgram();
            glUniform1f(0, 1.0f);
            glUniform1i(1, 1);
            fullScreenTri.draw();
            glEnable(GL_DEPTH_TEST);
        }

        // sRGB is broken in Dear ImGui
        //  glDisable(GL_FRAMEBUFFER_SRGB);
        ImGui::Extensions::FrameEnd();
        // glEnable(GL_FRAMEBUFFER_SRGB);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}