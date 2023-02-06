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
#include <intern/Scene/Scene.h>
#include <intern/Scene/Entity.h>
#include <intern/Texture/IO/png.h>
#include <intern/Texture/Texture.h>
#include <intern/Texture/TextureCube.h>
#include <intern/Window/Window.h>
#include <intern/Scene/Material.h>
#include <intern/Scene/Light.h>

int main()
{
    Context ctx{};
    Context::globalContext = &ctx;

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

    FullscreenTri fullScreenTri;
    ShaderProgram postProcessShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/General/postProcess.frag"}};
    Framebuffer internalFBO{WIDTH, HEIGHT, {{.internalFormat = GL_RGBA16F}}, true};

    const Cube cube{1.0f};
    const Mesh referenceHuman{MISC_PATH "/HumanScaleReference.obj"};
    const Texture gridTexture{MISC_PATH "/GridTexture.png", true, false};
    ShaderProgram simpleShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/simpleTexture.vert", SHADERS_PATH "/General/simpleTexture.frag"}};

    FogEffect fogEffect(WIDTH, HEIGHT);
    
    FullscreenTri tri = FullscreenTri();

    Texture tAlbedo(MISC_PATH "/YellowBrick_basecolor.tga", false, true);
    Texture tNormal(MISC_PATH "/YellowBrick_normal.tga", false, true);
    Texture tAttributes(MISC_PATH "/YellowBrick_attributes.tga", false, true);

    //----------------------- INIT SCENE

    Scene MainScene;
    MainScene.init();
	MainScene.setSkyExposure(0.5f);
	MainScene.sun()->setDirectionFromPolarCoord(glm::radians(45.0f), glm::radians(130.0f));

	Entity* testObject = MainScene.createEntity();
    testObject->setMaterial(new Material(MISC_PATH "/YellowBrick_basecolor.tga", MISC_PATH "/YellowBrick_normal.tga", MISC_PATH "/YellowBrick_attributes.tga"));
	testObject->setMesh(new Mesh(MISC_PATH "/Meshes/sphere.obj"));
	testObject->setPosition(glm::vec3(0,20,10));
	testObject->getMaterial()->setBaseColor(Color(1.0f, 1.0f, 0.0f));
	testObject->getMaterial()->setNormalIntensity(0.0f);

	Entity* testObject2 = MainScene.createEntity();
    testObject2->setMaterial(new Material(MISC_PATH "/YellowBrick_basecolor.tga", MISC_PATH "/YellowBrick_normal.tga", MISC_PATH "/YellowBrick_attributes.tga"));
	testObject2->setMesh(new Mesh(MISC_PATH "/Meshes/shadowtest.obj"));
	testObject2->setPosition(glm::vec3(10,20,10));
	testObject2->getMaterial()->setBaseColor(Color::White);
	testObject2->getMaterial()->setNormalIntensity(0.0f);

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

        

        glBindTextureUnit(1, terrainNormal.getTextureID());
        glBindTextureUnit(2, terrainMaterialIDs.getTextureID());
        glBindTextureUnit(3, heightTextureArray);
        glBindTextureUnit(4, diffuseTextureArray);
        glBindTextureUnit(5, normalTextureArray);
        glBindTextureUnit(6, ordTextureArray);
		MainScene.bind();
        cbt.draw(*cam.getView(), *cam.getProj() * *cam.getView());
        if(cbt.getSettings().drawOutline)
        {
            cbt.drawOutline(*cam.getProj() * *cam.getView());
        }

		MainScene.draw(cam, WIDTH, HEIGHT);

        //pbsShader.useProgram();
        //glUniformMatrix4fv(
        //    0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3{0.0f, 0.5f + groundOffsetAt00, 0.0f})));
        //glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        //glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        //cube.draw();


        

        glDisable(GL_DEPTH_TEST);
        // Post Processing
        {
            // fog
            const auto& hdrColorWithFogTex =
                fogEffect.execute(internalFBO.getColorTextures()[0], *internalFBO.getDepthTexture());

            // color management
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // glViewport()?
            // overwriting full screen anyways, dont need to clear
            glBindTextureUnit(0, hdrColorWithFogTex.getTextureID());
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
            if(ImGui::CollapsingHeader("Fog##settings"))
            {
                ImGui::Indent(5.0f);
                if(ImGui::CollapsingHeader("BasicFog##settings"))
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

				static float SunTemp = 5900.0f;
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

				ImGui::TextUnformatted("Sky");
				static float SkyExposure = 0.5f;
				if(ImGui::SliderFloat("Indirect Light", &SkyExposure, 0.0f, 4.0f))
                {
                    MainScene.setSkyExposure(SkyExposure);
                }
				static float SkyboxExposure = 1.0f;
				if(ImGui::SliderFloat("Skybox Exposure", &SkyboxExposure, 0.0f, 4.0f))
                {
                    MainScene.setSkyboxExposure(SkyboxExposure);
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