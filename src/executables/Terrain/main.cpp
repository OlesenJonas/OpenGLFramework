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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);

    //----------------------- INIT IMGUI & Input

    InputManager input(ctx);
    ctx.setInputManager(&input);
    input.setupCallbacks();

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
    const Texture testHeightmap{MISC_PATH "/CBT/testHeight.png", false, false};
    const Texture testNormal{MISC_PATH "/CBT/testNrm.png", false, false};

    const Cube cube{1.0f};
    const Mesh referenceHuman{MISC_PATH "/HumanScaleReference.obj"};
    const Texture gridTexture{MISC_PATH "/GridTexture.png", true, false};
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

        glBindTextureUnit(0, testHeightmap.getTextureID());
        static bool freezeCBTUpdate = false;
        if(!freezeCBTUpdate)
        {
            cbt.update(*cam.getProj() * *cam.getView(), {WIDTH, HEIGHT});
        }
        cbt.doSumReduction();
        cbt.writeIndirectCommands();

        glBindTextureUnit(1, testNormal.getTextureID());
        cbt.draw(*cam.getProj() * *cam.getView());
        static bool drawCBTOutline = false;
        if(drawCBTOutline)
        {
            cbt.drawOutline(*cam.getProj() * *cam.getView());
        }
        cbt.drawOverlay(cam.getAspect());

        simpleShader.useProgram();
        glBindTextureUnit(0, gridTexture.getTextureID());
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{0.0f, 0.5f, 0.0f})));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        cube.draw();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{1.0f, 0.0f, 0.0f})));
        referenceHuman.draw();

        // UI
        {
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Draw outline", &drawCBTOutline);
            ImGui::Separator();
            static float targetEdgeLength = 10.0f;
            if(ImGui::SliderFloat("Target edge length", &targetEdgeLength, 1.0f, 100.0f))
            {
                cbt.setTargetEdgeLength(targetEdgeLength);
            }
            static int globalSubdivLevel = 0;
            if(ImGui::SliderInt("Global subdiv level", &globalSubdivLevel, 0, cbt.getMaxTemplateLevel()))
            {
                cbt.setTemplateLevel(globalSubdivLevel);
            }
            ImGui::Checkbox("Freeze update", &freezeCBTUpdate);
            ImGui::End();
        }
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
                ImGui::Text("Additional subdiv level: %d", cbt.getTemplateLevel());
            }
            ImGui::End();
        }

        // sRGB is broken in Dear ImGui
        glDisable(GL_FRAMEBUFFER_SRGB);
        ImGui::Extensions::FrameEnd();
        glEnable(GL_FRAMEBUFFER_SRGB);

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