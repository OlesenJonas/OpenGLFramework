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
#include <intern/Terrain/CBTOptimized.h>
#include <intern/Texture/Texture.h>
#include <intern/Texture/TextureCube.h>
#include <intern/Window/Window.h>

struct UserPointerStruct
{
    CBTOptimized* cbt;
    bool updateCBTdynamically = false;
    CBTOptimized::Node hitNode{1, 0};
    glm::vec3 hitPoint;
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Context& ctx = *static_cast<Context*>(glfwGetWindowUserPointer(window));
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
    auto& userPointerStruct = *static_cast<UserPointerStruct*>(ctx.getUserPointer());
    if((key == GLFW_KEY_N || key == GLFW_KEY_M) && action == GLFW_PRESS)
    {
        if(!userPointerStruct.updateCBTdynamically)
        {
            CBTOptimized& cbt = *userPointerStruct.cbt;

            const CBTOptimized::Node hitNode = userPointerStruct.hitNode;

            if(hitNode.heapIndex != 0)
            {
                if(key == GLFW_KEY_N)
                {
                    cbt.splitNodeConforming(hitNode);
                }
                if(key == GLFW_KEY_M)
                {
                    cbt.mergeNodeConforming(hitNode);
                }
                cbt.doSumReduction();
                cbt.updateDrawData();
            }
        }
    }
    if(key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        auto& userPointerStruct = *static_cast<UserPointerStruct*>(ctx.getUserPointer());
        CBTOptimized& cbt = *userPointerStruct.cbt;
        userPointerStruct.updateCBTdynamically = !userPointerStruct.updateCBTdynamically;
        cbt.doSumReduction();
        cbt.updateDrawData();
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
        CBTOptimized& cbt = *userPointerStruct.cbt;

        userPointerStruct.hitNode = cbt.nodeFromPoint({planeHit.x, planeHit.z});
    }
}

int main()
{
    Context ctx{};

    //----------------------- INIT WINDOW

    int WIDTH = 1200;
    int HEIGHT = 800;

    GLFWwindow* window =
        initAndCreateGLFWWindow(WIDTH, HEIGHT, "Binary triangle tree test", {{GLFW_MAXIMIZED, GLFW_TRUE}});

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
    glDepthRange(0.0f, 1.0f);
    glClearDepth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // glEnable(GL_FRAMEBUFFER_SRGB);
    // for triangle vis mode
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

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

    Camera cam{ctx, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT)};
    ctx.setCamera(&cam);

    CBTOptimized cbt(10);
    UserPointerStruct userPointerStruct{};
    userPointerStruct.cbt = &cbt;
    ctx.setUserPointer(&userPointerStruct);

    Cube cube{0.01f};
    ShaderProgram simpleShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/simple.vert", SHADERS_PATH "/General/simple.frag"}};

    Texture tAlbedo(MISC_PATH "/YellowBrick_basecolor.tga", true);
    Texture tNormal(MISC_PATH "/YellowBrick_normal.tga", true);
    Texture tAttributes(MISC_PATH "/YellowBrick_attributes.tga", true);

    std::vector<std::string> cubeTextures = {
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/GridTexture.png",
        MISC_PATH "/GridTexture.png"};
    TextureCube tSky(cubeTextures);


    Cube matCube{1.0f};
    ShaderProgram pbsShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/pbs.vert", SHADERS_PATH "/General/pbs.frag"}};

    ShaderProgram skyShader{
    VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
    {SHADERS_PATH "/General/sky.vert", SHADERS_PATH "/General/sky.frag"}};
    FullscreenTri tri = FullscreenTri();

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
        glDepthFunc(GL_LESS);

        simpleShader.useProgram();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::translate(userPointerStruct.hitPoint)));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
        cube.draw();

        //----------------------- PBS
        {
            pbsShader.useProgram();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::vec3(0,1,0))));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*cam.getView()));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*cam.getProj()));
            glBindTextureUnit(1, tAlbedo.getTextureID());
            glBindTextureUnit(2, tNormal.getTextureID());
            glBindTextureUnit(3, tAttributes.getTextureID());
            matCube.draw();
        }

        //----------------------- CBT

        if(userPointerStruct.updateCBTdynamically)
        {
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
                userPointerStruct.hitPoint = planeHit;

                cbt.refineAroundPoint({planeHit.x, planeHit.z});
                cbt.updateDrawData();
            }
        }

        cbt.draw(*cam.getProj() * *cam.getView());

        //----------------------- Sky
        {
            glm::mat4 view = *cam.getView();
            view[3] = glm::vec4(0, 0, 0, 1);
            glm::mat4 proj = *cam.getProj();
            glm::mat4 inv = glm::inverse(proj * view);

            skyShader.useProgram();
            glBindTexture(GL_TEXTURE_CUBE_MAP, tSky.getTextureID());
            glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(inv));
            glDepthFunc(GL_EQUAL);
            tri.draw();
        }

        // UI
        {
            ImGui::Begin("CBT Info");
            if(userPointerStruct.hitNode.heapIndex != 0)
            {
                ImGui::Text("Update dynamically (D):\n%d", userPointerStruct.updateCBTdynamically);
                ImGui::Separator();
                const CBTOptimized::SameDepthNeighbourhood neighbourhood =
                    cbt.calculateSameDepthNeighbourhood(userPointerStruct.hitNode);
                ImGui::Text("Selected node: %u", userPointerStruct.hitNode.heapIndex);
                ImGui::Text("Neighbours:");
                ImGui::Indent(5.0f);
                ImGui::Text("Edge  : %u", neighbourhood.edge);
                ImGui::Text("Left  : %u", neighbourhood.left);
                ImGui::Text("Right : %u", neighbourhood.right);
                ImGui::Indent(-5.0f);
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