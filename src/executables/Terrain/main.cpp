#include <glad/glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/transform.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_opengl3.h>

#include <intern/Camera/Camera.h>
#include <intern/Context/Context.h>
#include <intern/InputManager/InputManager.h>
#include <intern/Mesh/Cube.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/Misc/ImGuiExtensions.h>
#include <intern/Misc/OpenGLErrorHandler.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Texture/Texture.h>
#include <intern/Window/Window.h>

struct UserPointerStruct
{
    CBTGPU* cbt;
    glm::vec2 hitPoint;
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Context& ctx = *static_cast<Context*>(glfwGetWindowUserPointer(window));
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
    auto& userPointerStruct = *static_cast<UserPointerStruct*>(ctx.getUserPointer());
    CBTGPU& cbt = *userPointerStruct.cbt;
    if(key == GLFW_KEY_K && action == GLFW_PRESS)
    {
        cbt.setTemplateLevel(cbt.getTemplateLevel() - 1);
    }
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        cbt.setTemplateLevel(cbt.getTemplateLevel() + 1);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // IsWindowHovered enough? or ImGui::getIO().WantCapture[Mouse/Key]
    if(ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        return;
    }

    Context& ctx = *static_cast<Context*>(glfwGetWindowUserPointer(window));
    // if((button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS)
    // {
    //     glfwGetCursorPos(window, &cbStruct->mousePos->x, &cbStruct->mousePos->y);
    // }
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        ctx.getCamera()->setMode(Camera::Mode::FLY);
    }
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        ctx.getCamera()->setMode(Camera::Mode::ORBIT);
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        auto& userPointerStruct = *static_cast<UserPointerStruct*>(ctx.getUserPointer());
        const glm::mat4 invView = glm::inverse(*ctx.getCamera()->getView());
        const glm::vec4 camOriginWorld = invView * glm::vec4(0, 0, 0, 1);
        const glm::vec4 camDirection = invView * glm::vec4(0, 0, -1, 0);
        if(camOriginWorld.y <= 0 || camDirection.y >= 0)
            return;

        const glm::mat4 invProjection = glm::inverse(*ctx.getCamera()->getProj());
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        const glm::vec2 mousePos = ctx.getInputManager()->getMousePos();
        const glm::vec4 cursorPosNDC =
            glm::vec4(2.0f * (mousePos.x / width) - 1.0, 2.0f * (1.0 - (mousePos.y / height)) - 1.0, -1, 1);
        glm::vec4 cursorPosView = invProjection * cursorPosNDC;
        cursorPosView /= cursorPosView.w;
        const glm::vec4 cursorPosWorld = invView * cursorPosView;
        const glm::vec4 cursorDirectionWorld = cursorPosWorld - camOriginWorld;

        const glm::vec4 planeHit =
            camOriginWorld + cursorDirectionWorld * (camOriginWorld.y / -cursorDirectionWorld.y);
        userPointerStruct.hitPoint = planeHit;
    }
}

int main()
{
    Context ctx{};

    //----------------------- INIT WINDOW

    int WIDTH = 1200;
    int HEIGHT = 800;

    GLFWwindow* window = initAndCreateGLFWWindow(WIDTH, HEIGHT, "Terrain", {{GLFW_MAXIMIZED, GLFW_TRUE}});

    ctx.setWindow(window);
    // disable VSYNC
    // glfwSwapInterval(0);

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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // glEnable(GL_FRAMEBUFFER_SRGB);

    //----------------------- INIT IMGUI & Input

    InputManager input(ctx);
    ctx.setInputManager(&input);
    input.setupCallbacks(keyCallback, mouseButtonCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    ImGui::StyleColorsDark();
    // platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    //----------------------- INIT REST

    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.01f, 1000.0f};
    ctx.setCamera(&cam);

    CBTGPU cbt(25);
    UserPointerStruct userPointerStruct{};
    userPointerStruct.cbt = &cbt;
    ctx.setUserPointer(&userPointerStruct);

    const Cube cube{1.0f};
    const Texture gridTexture{MISC_PATH "/GridTexture.png", true};
    ShaderProgram simpleShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/simpleTexture.vert", SHADERS_PATH "/General/simpleTexture.frag"}};

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 invView = glm::inverse(*ctx.getCamera()->getView());
        const glm::vec4 camOriginWorld = invView * glm::vec4(0, 0, 0, 1);
        const glm::vec4 camDirection = invView * glm::vec4(0, 0, -1, 0);
        if(camOriginWorld.y > 0 && camDirection.y < 0)
        {
            const glm::mat4 invProjection = glm::inverse(*ctx.getCamera()->getProj());
            int width = 0;
            int height = 0;
            glfwGetWindowSize(window, &width, &height);
            const glm::vec2 mousePos = ctx.getInputManager()->getMousePos();
            const glm::vec4 cursorPosNDC = glm::vec4(
                2.0f * (mousePos.x / width) - 1.0, 2.0f * (1.0 - (mousePos.y / height)) - 1.0, -1, 1);
            glm::vec4 cursorPosView = invProjection * cursorPosNDC;
            cursorPosView /= cursorPosView.w;
            const glm::vec4 cursorPosWorld = invView * cursorPosView;
            const glm::vec4 cursorDirectionWorld = cursorPosWorld - camOriginWorld;

            const glm::vec4 planeHit =
                camOriginWorld + cursorDirectionWorld * (camOriginWorld.y / -cursorDirectionWorld.y);
            userPointerStruct.hitPoint = {planeHit.x, planeHit.z};
        }
        cbt.update(userPointerStruct.hitPoint);
        cbt.doSumReduction();
        cbt.writeIndirectCommands();

        cbt.draw(*cam.getProj() * *cam.getView());
        cbt.drawOutline(*cam.getProj() * *cam.getView());

        simpleShader.useProgram();
        glBindTextureUnit(0, gridTexture.getTextureID());
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{0.0f, 0.5f, 0.0f})));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        cube.draw();

        // UI
        {
            ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("ImGui Measurement:");
            const ImGuiIO& io = ImGui::GetIO(); // NOLINT
            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                io.Framerate);
            if(ImGui::CollapsingHeader("CBT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextUnformatted("Update");
                ImGui::Indent(5.0f);
                ImGui::Text("Split: %.3f ms", cbt.getSplitTimer().timeMilliseconds());
                ImGui::Text("Merge: %.3f ms", cbt.getMergeTimer().timeMilliseconds());
                ImGui::Indent(-5.0f);
                ImGui::Text("Sum reduction : %.3f ms", cbt.getSumReductionTimer().timeMilliseconds());
                ImGui::Text("Indirect write: %.3f ms", cbt.getIndirectWriteTimer().timeMilliseconds());
                ImGui::Text("Drawing       : %.3f ms", cbt.getDrawTimer().timeMilliseconds());
                ImGui::Text("Global subdiv level: %d", cbt.getTemplateLevel());
            }
            ImGui::End();
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