#include <glad/glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/string_cast.hpp>
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
#include <intern/Scene/Entity.h>
#include <intern/Scene/Light.h>
#include <intern/Scene/Material.h>
#include <intern/Scene/ReflectionProbe.h>
#include <intern/Scene/Scene.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Terrain/Terrain.h>
#include <intern/Texture/IO/png.h>
#include <intern/Texture/Texture.h>
#include <intern/Texture/TextureCube.h>
#include <intern/Window/Window.h>

int main()
{
    Context ctx{};
    Context::globalContext = &ctx;

    //----------------------- INIT WINDOW

    GLFWwindow* window = initAndCreateGLFWWindow(1200, 800, "Terrain", {{GLFW_MAXIMIZED, GLFW_TRUE}});

    ctx.setWindow(window);
    // disable VSYNC
    // glfwSwapInterval(0);

    // In case window was set to start maximized, retrieve size for framebuffer here
    glfwGetWindowSize(window, &ctx.outputWidth, &ctx.outputHeight);

    ctx.internalWidth = ctx.outputWidth / 1;
    ctx.internalHeight = ctx.outputHeight / 1;

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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

    FullscreenTri fullScreenTri;
    ShaderProgram postProcessShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/General/postProcess.frag"}};
    Framebuffer internalFBO{ctx.internalWidth, ctx.internalHeight, {{.internalFormat = GL_R11F_G11F_B10F}}, true};
    // Framebuffer internalFBO{WIDTH, HEIGHT, {{.internalFormat = GL_RGBA32F}}, true};

    const std::vector<std::pair<std::string, float>> foldersAndSizes = {
        {"RockCliffs", 1.8}, //
        {"RockFlat", 1.5},   //
        {"Rubble", 2},       //
        {"Gravel", 1},       // //Scaling factor unknown, may be wrong
        {"RockyTrail", 2},   //
    };
    // Need to load as UI8 texture, which is not currently supported by the default texture load functionality
    auto imageData = png::read(MISC_PATH "/CBT/TerrainMaterialIDs.png", false, true);
    Terrain terrain{
        Texture{MISC_PATH "/CBT/TerrainHeight.png", false, false},
        Texture{MISC_PATH "/CBT/TerrainNormal.png", false, true},
        Texture{TextureDesc{
            .name = "Terrain Material IDs",
            .levels = 1,
            .width = static_cast<GLsizei>(imageData.width),
            .height = static_cast<GLsizei>(imageData.height),
            .internalFormat = GL_R8UI,
            .minFilter = GL_NEAREST,
            .magFilter = GL_NEAREST,
            .wrapS = GL_CLAMP_TO_EDGE,
            .wrapT = GL_CLAMP_TO_EDGE,
            .data = imageData.data.get(),
            .dataFormat = GL_RED_INTEGER,
            .dataType = GL_UNSIGNED_BYTE,
            .generateMips = false}},
        static_cast<uint8_t>(foldersAndSizes.size()),
        1024,
        1024,
        internalFBO};
    imageData.data.release();
    terrain.cbt.setTargetEdgeLength(7.0f);
    for(int i = 0; i < foldersAndSizes.size(); i++)
    {
        const auto& entry = foldersAndSizes[i];
        terrain.setMaterial(
            i,
            png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Diffuse.png", true, true),
            png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Normal.png", false, true),
            png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/ORD.png", false, true),
            png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Height.png", false, true),
            entry.second);
    }
    terrain.finalizeMaterials();
    constexpr float groundOffsetAt00 = 12.0f;

    Camera cam{static_cast<float>(ctx.internalWidth) / static_cast<float>(ctx.internalHeight), 0.1f, 1000.0f};
    ctx.setCamera(&cam);
    cam.move({0.f, groundOffsetAt00, 0.f});

    // const Cube cube{1.0f};
    // const Texture gridTexture{MISC_PATH "/GridTexture.png", true, false};
    // ShaderProgram simpleShader{
    //     VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
    //     {SHADERS_PATH "/General/simpleTexture.vert", SHADERS_PATH "/General/simpleTexture.frag"}};

    bool applyFog = true;
    FogEffect fogEffect(Context::globalContext->internalWidth, Context::globalContext->internalHeight);
    GPUTimer<128> fogPassTimer{"Fog"};

    FullscreenTri tri = FullscreenTri();

    Texture tAlbedo(MISC_PATH "/YellowBrick_basecolor.tga", false, true);
    Texture tNormal(MISC_PATH "/YellowBrick_normal.tga", false, true);
    Texture tAttributes(MISC_PATH "/YellowBrick_attributes.tga", false, true);

    //----------------------- INIT SCENE

    Scene MainScene;
    MainScene.init();
    MainScene.setSkyExposure(0.7f);
    MainScene.sun()->setDirectionFromPolarCoord(glm::radians(47.32f), glm::radians(278.351f));
    MainScene.sun()->setColor(Color::fromTemperature(4900.0f));

    ReflectionProbe* probe = MainScene.createReflectionProbe();
    probe->setActive(false);
    probe->setVisibility(false);

    Entity* testObject2 = MainScene.createEntity();
    testObject2->setMaterial(new Material(
        MISC_PATH "/YellowBrick_basecolor.tga",
        MISC_PATH "/YellowBrick_normal.tga",
        MISC_PATH "/YellowBrick_attributes.tga"));
    testObject2->setMesh(new Mesh(MISC_PATH "/Meshes/shadowtest.obj"));
    testObject2->setPosition(glm::vec3(10, 20, 10));
    testObject2->getMaterial()->setBaseColor(Color::White);
    testObject2->getMaterial()->setNormalIntensity(1.0f);

    Entity* human = MainScene.createEntity();
    human->setMaterial(new Material(
        MISC_PATH "/GridTexture.png", MISC_PATH "/YellowBrick_normal.tga", MISC_PATH "/GridTexture.png"));
    human->setMesh(new Mesh(MISC_PATH "/HumanScaleReference.obj"));
    human->setPosition(glm::vec3(5, 10.33, -11));
    human->getMaterial()->setBaseColor(Color::White);
    human->getMaterial()->setNormalIntensity(0.0f);

    std::vector<Entity*> pbrEntities;
    for(int x = 0; x < 2; ++x)
    {
        for(int y = 0; y < 8; ++y)
        {
            Entity* sphere = MainScene.createEntity();
            sphere->setMaterial(
                new Material(MISC_PATH "/white.jpg", MISC_PATH "/white.jpg", MISC_PATH "/white.jpg"));
            sphere->setMesh(new Mesh(MISC_PATH "/Meshes/sphere2.obj"));
            sphere->setPosition(glm::vec3(5 + (y * 1.4f), 21.5 + (x * 1.5f), 10));
            sphere->getMaterial()->setBaseColor(Color(0.063f, 0.22f, 0.31f));
            // sphere->getMaterial()->setBaseColor(Color(0.0f, 0.0f, 0.0f));
            sphere->getMaterial()->setNormalIntensity(0.0f);
            sphere->getMaterial()->setRoughness(std::max(0.05f, (float)y * (1.0f / 8.0f)));
            sphere->getMaterial()->setMetallic(x);

            pbrEntities.emplace_back(sphere);
        }
    }

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

        terrain.update();

        MainScene.prepass(terrain.cbt);
        MainScene.bind();

        internalFBO.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        terrain.draw(internalFBO);

        MainScene.draw(cam, Context::globalContext->internalWidth, Context::globalContext->internalHeight);

        // pbsShader.useProgram();
        // glUniformMatrix4fv(
        //     0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{0.0f, 0.5f + groundOffsetAt00,
        //     0.0f})));
        // glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        // glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        // cube.draw();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Post Processing");
        glDisable(GL_DEPTH_TEST);
        // Post Processing
        {
            // fog
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Fog");
            fogPassTimer.start();
            const auto& hdrColorTex =
                applyFog ? fogEffect.execute(internalFBO.getColorTextures()[0], *internalFBO.getDepthTexture())
                         : internalFBO.getColorTextures()[0];
            fogPassTimer.end();
            fogPassTimer.evaluate();
            glPopDebugGroup();

            // color management
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, Context::globalContext->outputWidth, Context::globalContext->outputHeight);
            // overwriting full screen anyways, dont need to clear
            glBindTextureUnit(0, hdrColorTex.getTextureID());
            postProcessShader.useProgram();
            fullScreenTri.draw();
        }
        glPopDebugGroup();

        // draw CBT overlay as part of UI
        terrain.cbt.drawOverlay(cam.getAspect());

        // UI
        {
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            if(ImGui::CollapsingHeader("Terrain##settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                terrain.drawSettings();
            }
            if(ImGui::CollapsingHeader("Fog##settings"))
            {
                ImGui::Indent(10.0f);
                ImGui::Checkbox("Apply fog", &applyFog);
                if(applyFog)
                {
                    fogEffect.drawUI();
                }
                ImGui::Indent(-10.0f);
            }
            if(ImGui::CollapsingHeader("Camera##settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // if(ImGui::Button("Write out position"))
                // {
                //     std::cout << glm::to_string(cam.getPosition()) << "\n";
                //     std::cout << glm::to_string(cam.getRotation()) << "\n";
                //     std::cout << cam.getRadius() << std::endl;
                // }
                // ImGui::SameLine();
                ImGui::TextUnformatted("Load camera position");
                if(ImGui::Button("#1"))
                {
                    cam.setPosition(glm::vec3(75.703384, 38.853016, -45.770397));
                    cam.setRotation(2.285002f, 1.215797f);
                    cam.setRadius(1.0f);
                }
                ImGui::SameLine();
                if(ImGui::Button("#2"))
                {
                    cam.setPosition(glm::vec3(-1.504049, 12.790466, -4.082536));
                    cam.setRotation(3.334999f, 1.900796f);
                    cam.setRadius(1.0f);
                }
                ImGui::SameLine();
                if(ImGui::Button("#3"))
                {
                    cam.setPosition(glm::vec3(31.238586, 13.624129, -42.391911));
                    cam.setRotation(4.814977f, 1.210797f);
                    cam.setRadius(1.0f);
                }
                static float exposure = 1.0f;
                if(ImGui::SliderFloat("Simple exposure", &exposure, 0.1f, 2.0f))
                {
                    postProcessShader.useProgram();
                    glUniform1f(0, exposure);
                }
            }
            if(ImGui::CollapsingHeader("Scene##settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextUnformatted("Sun");
                ImGui::Indent(10.0f);

                static float SunIntensity = 1.0f;
                if(ImGui::SliderFloat("Sun Intensity", &SunIntensity, 0.0f, 100.0f))
                {
                    MainScene.sun()->setIntensity(SunIntensity);
                }

                static float SunTemp = 4900.0f;
                if(ImGui::SliderFloat("Temperature", &SunTemp, 1000.0f, 15000.0f))
                {
                    MainScene.sun()->setColor(Color::fromTemperature(SunTemp));
                }

                static float Theta = 47.32f;
                static float Phi = 278.351f;
                if(ImGui::SliderFloat("Theta", &Theta, -90.0f, 90.0f))
                {
                    MainScene.sun()->setDirectionFromPolarCoord(glm::radians(Theta), glm::radians(Phi));
                }
                if(ImGui::SliderFloat("Phi", &Phi, 0.0f, 360.0f))
                {
                    MainScene.sun()->setDirectionFromPolarCoord(glm::radians(Theta), glm::radians(Phi));
                }

                ImGui::Indent(-10.0f);
                ImGui::TextUnformatted("Sky");
                ImGui::Indent(10.0f);

                static float SkyExposure = 0.7f;
                if(ImGui::SliderFloat("Indirect Light", &SkyExposure, 0.0f, 10.0f))
                {
                    MainScene.setSkyExposure(SkyExposure);
                }
                // static float SkyboxExposure = 1.0f;
                // if(ImGui::SliderFloat("Skybox Exposure", &SkyboxExposure, 0.0f, 4.0f))
                //{
                //     MainScene.setSkyboxExposure(SkyboxExposure);
                // }

                ImGui::Indent(-10.0f);
                ImGui::TextUnformatted("Reflecion Probe");
                ImGui::Indent(10.0f);

                static bool ReflectionProbeActive = false;
                if(ImGui::Checkbox("Active", &ReflectionProbeActive))
                {
                    MainScene.reflectionProbe()->setActive(ReflectionProbeActive);
                }
                static bool ShowReflectionProbes = false;
                if(ImGui::Checkbox("Visible", &ShowReflectionProbes))
                {
                    MainScene.reflectionProbe()->setVisibility(ShowReflectionProbes);
                    MainScene.sun()->markDirty();
                }
                static bool RealtimeReflection = false;
                if(ImGui::Checkbox("Realtime Reflections", &RealtimeReflection))
                {
                    MainScene.reflectionProbe()->setMode(
                        RealtimeReflection ? ReflectionProbeMode_Realtime : ReflectionProbeMode_Static);
                }

                static bool ShowShadingTest = true;
                if(ImGui::Checkbox("Show Shading Test", &ShowShadingTest))
                {
                    for(auto& entity : pbrEntities)
                    {
                        entity->setVisibility(ShowShadingTest);
                    }
                    testObject2->setVisibility(ShowShadingTest);
                    MainScene.sun()->markDirty();
                }
            }
            ImGui::End();
        }
        {
            ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("ImGui Measurement:");
            const ImGuiIO& io = ImGui::GetIO(); // NOLINT
            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, io.Framerate);
            if(ImGui::CollapsingHeader("Terrain##timers", ImGuiTreeNodeFlags_DefaultOpen))
            {
                terrain.drawTimers();
            }
            if(ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Total: %.3f ms", fogPassTimer.timeMilliseconds());
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