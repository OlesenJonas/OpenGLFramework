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
    ImGui::StyleColorsDark();
    // platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    //----------------------- INIT REST

    CBTGPU cbt(25);
    cbt.setTargetEdgeLength(7.0f);
    const Texture terrainHeightmap{MISC_PATH "/CBT/TerrainHeight.png", false, false};
    glTextureParameteri(terrainHeightmap.getTextureID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(terrainHeightmap.getTextureID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const Texture terrainNormal{MISC_PATH "/CBT/TerrainNormal.png", false, true};
    const Texture terrainMacroColor{MISC_PATH "/CBT/TerrainMacroColor.png", true, true};
    constexpr float groundOffsetAt00 = 38.0f;

    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.01f, 1000.0f};
    ctx.setCamera(&cam);
    cam.move({0.f, groundOffsetAt00, 0.f});

    FullscreenTri fullScreenTri;
    ShaderProgram postProcessShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/General/postProcess.frag"}};
    Framebuffer internalFBO{WIDTH, HEIGHT, {GL_RGBA16F}, true};

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

        internalFBO.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glBindTextureUnit(0, terrainHeightmap.getTextureID());
        if(!cbt.getSettings().freezeUpdates)
        {
            cbt.update(*cam.getProj() * *cam.getView(), {WIDTH, HEIGHT});
        }
        cbt.doSumReduction();
        cbt.writeIndirectCommands();

        glBindTextureUnit(1, terrainNormal.getTextureID());
        glBindTextureUnit(2, terrainMacroColor.getTextureID());
        cbt.draw(*cam.getProj() * *cam.getView());
        if(cbt.getSettings().drawOutline)
        {
            cbt.drawOutline(*cam.getProj() * *cam.getView());
        }

        simpleShader.useProgram();
        glBindTextureUnit(0, gridTexture.getTextureID());
        glUniformMatrix4fv(
            0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{0.0f, 0.5f + groundOffsetAt00, 0.0f})));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        cube.draw();
        glUniformMatrix4fv(
            0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{1.0f, 0.0f + groundOffsetAt00, 0.0f})));
        referenceHuman.draw();

        glDisable(GL_DEPTH_TEST);
        // Post Processing
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // overwriting full screen anyways, dont need to clear
            glBindTextureUnit(0, internalFBO.getColorTextures()[0].getTextureID());
            postProcessShader.useProgram();
            fullScreenTri.draw();
        }

        // draw CBT overlay as part of UI
        cbt.drawOverlay(cam.getAspect());

        // UI
        {
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Separator();
            if(ImGui::CollapsingHeader("CBT##settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                cbt.drawUI();
            }
            if(ImGui::CollapsingHeader("Camera##settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static float exposure = 1.0f;
                if(ImGui::SliderFloat("Simple exposure", &exposure, 0.1f, 2.0f))
                {
                    postProcessShader.useProgram();
                    glUniform1f(0, exposure);
                }
            }
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
        // glDisable(GL_FRAMEBUFFER_SRGB);
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