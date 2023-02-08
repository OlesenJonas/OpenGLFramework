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
#include <intern/Scene/Entity.h>
#include <intern/Scene/Light.h>
#include <intern/Scene/Material.h>
#include <intern/Scene/ReflectionProbe.h>
#include <intern/Scene/Scene.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Texture/IO/png.h>
#include <intern/Texture/Texture.h>
#include <intern/Texture/TextureCube.h>
#include <intern/Window/Window.h>

int main()
{
    Context ctx{};
    Context::globalContext = &ctx;

    //----------------------- INIT WINDOW

    int WIDTH = 1200;
    int HEIGHT = 800;
    int TRUE_WIDTH = 0;
    int TRUE_HEIGHT = 0;

    GLFWwindow* window = initAndCreateGLFWWindow(WIDTH, HEIGHT, "Terrain", {{GLFW_MAXIMIZED, GLFW_TRUE}});

    ctx.setWindow(window);
    // disable VSYNC
    // glfwSwapInterval(0);

    // In case window was set to start maximized, retrieve size for framebuffer here
    glfwGetWindowSize(window, &WIDTH, &HEIGHT);

    TRUE_WIDTH = WIDTH;
    TRUE_HEIGHT = HEIGHT;
    WIDTH /= 1;
    HEIGHT /= 1;

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
    Framebuffer internalFBO{WIDTH, HEIGHT, {{.internalFormat = GL_R11F_G11F_B10F}}, true};
    // Framebuffer internalFBO{WIDTH, HEIGHT, {{.internalFormat = GL_RGBA32F}}, true};

    CBTGPU cbt(25, WIDTH, HEIGHT, internalFBO);
    cbt.setTargetEdgeLength(7.0f);
    const Texture terrainHeightmap{MISC_PATH "/CBT/TerrainHeight.png", false, false};
    glTextureParameteri(terrainHeightmap.getTextureID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(terrainHeightmap.getTextureID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const Texture terrainNormal{MISC_PATH "/CBT/TerrainNormal.png", false, true};
    // const Texture terrainMaterialIDs{MISC_PATH "/CBT/TerrainMaterialIDs.png", false, false};
    // Need to load as UI8 texture, which is not currently supported by the default texture load functionality
    auto imageData = png::read(MISC_PATH "/CBT/TerrainMaterialIDs.png", false, true);
    const Texture terrainMaterialIDs{TextureDesc{
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
        .generateMips = false}};
    imageData.data.release();
    constexpr float groundOffsetAt00 = 12.0f;

    // Load material textures into Texture Array
    const std::vector<std::pair<std::string, float>> foldersAndSizes = {
        {"RockCliffs", 1.8}, //
        {"RockFlat", 1.5},   //
        {"Rubble", 2},       //
        {"Gravel", 1},       // //Scaling factor unknown, may be wrong
        {"RockyTrail", 2},   //
    };
    GLfloat maxAniso = 0;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    GLuint diffuseTextureArray;
    GLuint normalTextureArray;
    GLuint heightTextureArray;
    GLuint ordTextureArray;
    GLuint textureArrayInfoSSBO;
    {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &diffuseTextureArray);
        glObjectLabel(GL_TEXTURE, diffuseTextureArray, -1, "Diffuse Texture Array");
        glTextureStorage3D(diffuseTextureArray, 8, GL_SRGB8, 1024, 1024, foldersAndSizes.size());
        glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(diffuseTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(diffuseTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameterf(diffuseTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        for(int i = 0; i < foldersAndSizes.size(); i++)
        {
            const auto& entry = foldersAndSizes[i];
            auto tempDiffuseData =
                png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Diffuse.png", true, true);
            assert(tempDiffuseData.channels == Texture::Channels::RGB);
            assert(tempDiffuseData.pixelType == Texture::PixelType::UCHAR_SRGB);
            glTextureSubImage3D(
                diffuseTextureArray,
                0,
                0,
                0,
                i,
                1024,
                1024,
                1,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                tempDiffuseData.data.get());
        }
        glGenerateTextureMipmap(diffuseTextureArray);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &normalTextureArray);
        glObjectLabel(GL_TEXTURE, normalTextureArray, -1, "Normal Texture Array");
        glTextureStorage3D(normalTextureArray, 8, GL_RGB8, 1024, 1024, foldersAndSizes.size());
        glTextureParameteri(normalTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(normalTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(normalTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(normalTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(normalTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameterf(normalTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        for(int i = 0; i < foldersAndSizes.size(); i++)
        {
            const auto& entry = foldersAndSizes[i];
            auto tempDiffuseData =
                png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Normal.png", false, true);
            assert(tempDiffuseData.channels == Texture::Channels::RGB);
            assert(tempDiffuseData.pixelType == Texture::PixelType::UCHAR);
            glTextureSubImage3D(
                normalTextureArray,
                0,
                0,
                0,
                i,
                1024,
                1024,
                1,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                tempDiffuseData.data.get());
        }
        glGenerateTextureMipmap(normalTextureArray);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &heightTextureArray);
        glObjectLabel(GL_TEXTURE, heightTextureArray, -1, "Height Texture Array");
        glTextureStorage3D(heightTextureArray, 8, GL_R8, 1024, 1024, foldersAndSizes.size());
        glTextureParameteri(heightTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(heightTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(heightTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(heightTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(heightTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameterf(heightTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        for(int i = 0; i < foldersAndSizes.size(); i++)
        {
            const auto& entry = foldersAndSizes[i];
            auto tempDiffuseData =
                png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/Height.png", false, true);
            assert(tempDiffuseData.channels == Texture::Channels::R);
            assert(tempDiffuseData.pixelType == Texture::PixelType::UCHAR);
            glTextureSubImage3D(
                heightTextureArray,
                0,
                0,
                0,
                i,
                1024,
                1024,
                1,
                GL_RED,
                GL_UNSIGNED_BYTE,
                tempDiffuseData.data.get());
        }
        glGenerateTextureMipmap(heightTextureArray);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &ordTextureArray);
        glObjectLabel(GL_TEXTURE, ordTextureArray, -1, "ORD Texture Array");
        glTextureStorage3D(ordTextureArray, 8, GL_RGB8, 1024, 1024, foldersAndSizes.size());
        glTextureParameteri(ordTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(ordTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(ordTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(ordTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(ordTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameterf(ordTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        for(int i = 0; i < foldersAndSizes.size(); i++)
        {
            const auto& entry = foldersAndSizes[i];
            auto tempDiffuseData =
                png::read(MISC_PATH "/CBT/Textures/" + entry.first + "/ORD.png", false, true);
            assert(tempDiffuseData.channels == Texture::Channels::RGB);
            assert(tempDiffuseData.pixelType == Texture::PixelType::UCHAR);
            glTextureSubImage3D(
                ordTextureArray,
                0,
                0,
                0,
                i,
                1024,
                1024,
                1,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                tempDiffuseData.data.get());
        }
        glGenerateTextureMipmap(ordTextureArray);

        // Store texture info in ssbo (no UBO because I dont want the wasted space of a scalar array with
        // std140)
        std::vector<float> data;
        for(const auto& entry : foldersAndSizes)
        {
            data.push_back(entry.second);
        }
        glCreateBuffers(1, &textureArrayInfoSSBO);
        glNamedBufferStorage(textureArrayInfoSSBO, data.size() * sizeof(data[0]), data.data(), 0);
        glObjectLabel(GL_BUFFER, textureArrayInfoSSBO, -1, "Texture Array Info Buffer");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, textureArrayInfoSSBO);
    }

    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 1000.0f};
    ctx.setCamera(&cam);
    cam.move({0.f, groundOffsetAt00, 0.f});

    //const Cube cube{1.0f};
    //const Texture gridTexture{MISC_PATH "/GridTexture.png", true, false};
    //ShaderProgram simpleShader{
    //    VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
    //    {SHADERS_PATH "/General/simpleTexture.vert", SHADERS_PATH "/General/simpleTexture.frag"}};

    bool applyFog = true;
    FogEffect fogEffect(WIDTH, HEIGHT);
    GPUTimer<128> fogPassTimer{"Fog"};

    FullscreenTri tri = FullscreenTri();

    Texture tAlbedo(MISC_PATH "/YellowBrick_basecolor.tga", false, true);
    Texture tNormal(MISC_PATH "/YellowBrick_normal.tga", false, true);
    Texture tAttributes(MISC_PATH "/YellowBrick_attributes.tga", false, true);

    //----------------------- INIT SCENE

    Scene MainScene;
    MainScene.init();
    MainScene.setSkyExposure(0.7f);
    MainScene.sun()->setDirectionFromPolarCoord(glm::radians(45.0f), glm::radians(130.0f));
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
    testObject2->setVisibility(false);

	Entity* human = MainScene.createEntity();
    human->setMaterial(new Material(
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/YellowBrick_normal.tga",
        MISC_PATH "/GridTexture.png"));
    human->setMesh(new Mesh(MISC_PATH "/HumanScaleReference.obj"));
    human->setPosition(glm::vec3(5, 10.33, -11));
    human->getMaterial()->setBaseColor(Color::White);
    human->getMaterial()->setNormalIntensity(0.0f);

	std::vector<Entity*> pbrEntities;
	for (int x = 0; x < 2; ++x)
	{
		for (int y = 0; y < 8; ++y)
		{
			Entity* sphere = MainScene.createEntity();
			sphere->setMaterial(new Material(
				MISC_PATH "/white.jpg",
				MISC_PATH "/white.jpg",
				MISC_PATH "/white.jpg"));
			sphere->setMesh(new Mesh(MISC_PATH "/Meshes/sphere2.obj"));
			sphere->setPosition(glm::vec3(5 + (y * 1.4f), 21.5 + (x * 1.5f), 10));
			sphere->getMaterial()->setBaseColor(Color(0.063f, 0.22f, 0.31f));
			//sphere->getMaterial()->setBaseColor(Color(0.0f, 0.0f, 0.0f));
			sphere->getMaterial()->setNormalIntensity(0.0f);
			sphere->getMaterial()->setRoughness(std::max(0.05f, (float)y * (1.0f / 8.0f)));
			sphere->getMaterial()->setMetallic(x);

			pbrEntities.emplace_back(sphere);
			sphere->setVisibility(false);
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

        glBindTextureUnit(0, terrainHeightmap.getTextureID());
        if(!cbt.getSettings().freezeUpdates)
        {
            cbt.update(*cam.getProj() * *cam.getView(), {WIDTH, HEIGHT});
        }
        cbt.doSumReduction();
        cbt.writeIndirectCommands();

        MainScene.prepass(cbt);
        MainScene.bind();

        internalFBO.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        glBindTextureUnit(0, terrainHeightmap.getTextureID());
        glBindTextureUnit(1, terrainNormal.getTextureID());
        glBindTextureUnit(2, terrainMaterialIDs.getTextureID());
        glBindTextureUnit(3, heightTextureArray);
        glBindTextureUnit(4, diffuseTextureArray);
        glBindTextureUnit(5, normalTextureArray);
        glBindTextureUnit(6, ordTextureArray);
        cbt.draw(*cam.getView(), *cam.getProj(), *cam.getProj() * *cam.getView());
        if(cbt.getSettings().drawOutline)
        {
            cbt.drawOutline(*cam.getProj() * *cam.getView());
        }

        MainScene.draw(cam, WIDTH, HEIGHT);

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
                applyFog
                    ? fogEffect.execute(internalFBO.getColorTextures()[0], *internalFBO.getDepthTexture())
                    : internalFBO.getColorTextures()[0];
            fogPassTimer.end();
            fogPassTimer.evaluate();
            glPopDebugGroup();

            // color management
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, TRUE_WIDTH, TRUE_HEIGHT);
            // overwriting full screen anyways, dont need to clear
            glBindTextureUnit(0, hdrColorTex.getTextureID());
            postProcessShader.useProgram();
            fullScreenTri.draw();
        }
        glPopDebugGroup();

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
            if(ImGui::CollapsingHeader("Fog##settings"))
            {
                ImGui::Indent(5.0f);
                ImGui::Checkbox("Apply fog", &applyFog);
                if(applyFog)
                {
                    fogEffect.drawUI();
                }
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

                static float Theta = 45.0f;
                static float Phi = 130.0f;
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
                //static float SkyboxExposure = 1.0f;
                //if(ImGui::SliderFloat("Skybox Exposure", &SkyboxExposure, 0.0f, 4.0f))
                //{
                //    MainScene.setSkyboxExposure(SkyboxExposure);
                //}

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

				static bool ShowShadingTest = false;
                if(ImGui::Checkbox("Show Shading Test", &ShowShadingTest))
                {
                    for (auto& entity : pbrEntities)
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