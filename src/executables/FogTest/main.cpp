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
#include <intern/PostProcessEffects/Fog/Fog.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Texture/Texture.h>
#include <intern/Window/Window.h>

int main()
{
    Context ctx{};
    Context::globalContext = &ctx;

    //----------------------- INIT WINDOW

    int WIDTH = 1200;
    int HEIGHT = 800;

    GLFWwindow* window =
        initAndCreateGLFWWindow(WIDTH, HEIGHT, EXECUTABLE_NAME, {{GLFW_MAXIMIZED, GLFW_TRUE}});

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

    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 300.0f};
    ctx.setCamera(&cam);

    Framebuffer internalFBO{WIDTH, HEIGHT, {{.internalFormat = GL_RGBA16F}}, true};

    const Cube cube{1.0f};
    const Mesh geosphere{MISC_PATH "/Geosphere.obj"};
    constexpr int gridRes = 20;
    constexpr float gridSpacing = 5.0f;
    std::vector<glm::mat4> cubeTransforms;
    std::vector<glm::vec3> cubeColors;
    std::vector<glm::mat4> sphereTransforms;
    std::vector<glm::vec3> sphereColors;
    {
        // srand(0); //Explicitly want the same results every run
        for(int x = 0; x < gridRes; x++)
        {
            for(int z = 0; z < gridRes; z++)
            {
                glm::vec3 position = glm::vec3{x - 0.5f * gridRes, 0.0f, z - 0.5f * gridRes};
                position +=
                    0.7f * glm::vec3{rand() / float(RAND_MAX) - 0.5f, 0.f, rand() / float(RAND_MAX) - 0.5f};
                position *= gridSpacing;

                const glm::vec3 scale{
                    glm::mix(0.5f, 1.0f, rand() / float(RAND_MAX)),
                    glm::mix(0.8f, 2.0f, rand() / float(RAND_MAX)),
                    glm::mix(0.5f, 1.0f, rand() / float(RAND_MAX))};

                const glm::mat4 transform =
                    glm::translate(position) *                                                      //
                    glm::rotate(float(rand() / float(RAND_MAX) * M_PI), glm::vec3{0.f, 1.f, 0.f}) * //
                    glm::scale(scale) *                                                             //
                    glm::translate(glm::vec3{0.0f, 0.5f, 0.0f});

                cubeTransforms.emplace_back(transform);
            }
        }
        // https://gist.github.com/983/e170a24ae8eba2cd174f
        auto hsvToRGB = [](glm::vec3 c) -> glm::vec3
        {
            glm::vec4 K = glm::vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            glm::vec3 p = glm::abs(glm::fract(glm::vec3{c.x, c.x, c.x} + glm::vec3(K)) * 6.0f - K.w);
            return c.z *
                   mix(glm::vec3{K.x, K.x, K.x}, glm::clamp(p - K.x, glm::vec3(0.0), glm::vec3(1.0)), c.y);
        };
        for(int i = 0; i < gridRes * gridRes; i++)
        {
            const float h = rand() / float(RAND_MAX);
            const float s = 0.6f;
            const float v = 1.0f;

            cubeColors.emplace_back(hsvToRGB({h, s, v}));
        }

        for(int x = 0; x < gridRes - 1; x++)
        {
            for(int z = 0; z < gridRes - 1; z++)
            {
                glm::vec3 position = glm::vec3{x - 0.5f * gridRes, 0.0f, z - 0.5f * gridRes};
                position +=
                    0.4f * glm::vec3{rand() / float(RAND_MAX) - 0.5f, 0.f, rand() / float(RAND_MAX) - 0.5f};
                position *= gridSpacing;
                position += glm::vec3{0.5f, 0.f, 0.5f} * gridSpacing;
                position.y += glm::mix(0.5f, 2.0f, rand() / float(RAND_MAX));

                const glm::vec3 scale{glm::mix(0.2f, 0.5f, rand() / float(RAND_MAX))};

                const glm::mat4 transform = glm::translate(position) * glm::scale(scale);

                if(rand() > 0.8f * RAND_MAX)
                    sphereTransforms.emplace_back(transform);
            }
        }

        for(int i = 0; i < sphereTransforms.size(); i++)
        {
            const float h = rand() / float(RAND_MAX);

            sphereColors.emplace_back(
                hsvToRGB({h, 1.0f, 1.0f}) * glm::mix(1.0f, 10.0f, rand() / float(RAND_MAX)));
        }
    }

    const Texture gridTexture{MISC_PATH "/GridTexture.png", true, false};
    const Texture borderTexture{MISC_PATH "/border.png", false, true};
    ShaderProgram prototypeGridShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/prototypeGrid.vert", SHADERS_PATH "/General/prototypeGrid.frag"}};
    ShaderProgram emissiveShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/simple.vert", SHADERS_PATH "/General/simpleColor.frag"}};

    FullscreenTri fullScreenTri;
    ShaderProgram postProcessShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/General/postProcess.frag"}};

    FogEffect fogEffect(WIDTH, HEIGHT);

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

        // draw scene
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Scene Rendering");
        {
            prototypeGridShader.useProgram();
            glBindTextureUnit(0, borderTexture.getTextureID());
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));

            // ground
            const float extent = 100.0f;
            const glm::mat4 groundTransform = glm::scale(glm::vec3{2 * extent, 0.1f, 2 * extent}) *
                                              glm::translate(glm::vec3(0.f, -0.5f, 0.0f));
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(groundTransform));
            glUniform3fv(3, 1, glm::value_ptr(glm::vec3{1.0f}));
            cube.draw();
            glm::mat4 sideTransform = glm::translate(glm::vec3{-extent, 0.0f, 0.0f}) *
                                      glm::scale(glm::vec3{0.1f, 10.0f, 2 * extent}) *
                                      glm::translate(glm::vec3{-0.5f, 0.5f, 0.0f});
            for(int i = 0; i < 4; i++)
            {
                sideTransform =
                    glm::rotate(glm::radians(i * 90.0f), glm::vec3{0.f, 1.f, 0.f}) * sideTransform;
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(sideTransform));
                glUniform3fv(3, 1, glm::value_ptr(glm::vec3{1.0f}));
                cube.draw();
            }

            for(int i = 0; i < cubeTransforms.size(); i++)
            {
                glUniform3fv(3, 1, glm::value_ptr(cubeColors[i]));
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(cubeTransforms[i]));
                cube.draw();
            }

            emissiveShader.useProgram();
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));

            for(int i = 0; i < sphereTransforms.size(); i++)
            {
                glUniform3fv(3, 1, glm::value_ptr(sphereColors[i]));
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(sphereTransforms[i]));
                geosphere.draw();
            }
        }
        glPopDebugGroup();

        glDisable(GL_DEPTH_TEST);
        // Post Processing
        {
            // fog
            const auto& hdrColorWithFogTex =
                fogEffect.execute(internalFBO.getColorTextures()[0], *internalFBO.getDepthTexture());
            glViewport(0, 0, WIDTH, HEIGHT);

            // color management
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // overwriting full screen anyways, dont need to clear
            glBindTextureUnit(0, hdrColorWithFogTex.getTextureID());
            postProcessShader.useProgram();
            fullScreenTri.draw();
        }

        // UI
        {
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Separator();
            if(ImGui::CollapsingHeader("Fog##settings"))
            {
                ImGui::Indent(5.0f);
                fogEffect.drawUI();
                // basicFogEffect.drawUI();
                ImGui::Indent(-5.0f);
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
            // if(ImGui::CollapsingHeader("CBT", ImGuiTreeNodeFlags_DefaultOpen))
            // {
            //     ImGui::TextUnformatted("Update");
            //     ImGui::Indent(5.0f);
            //     ImGui::Text("Split: %.3f ms", cbt.getSplitTimer().timeMilliseconds());
            //     ImGui::Text("Merge: %.3f ms", cbt.getMergeTimer().timeMilliseconds());
            //     ImGui::Indent(-5.0f);
            //     ImGui::Text("Sum reduction : %.3f ms", cbt.getSumReductionTimer().timeMilliseconds());
            //     ImGui::Text("Indirect write: %.3f ms", cbt.getIndirectWriteTimer().timeMilliseconds());
            //     ImGui::Text("Drawing       : %.3f ms", cbt.getDrawTimer().timeMilliseconds());
            //     ImGui::Text("Additional subdiv level: %d", cbt.getTemplateLevel());
            // }
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