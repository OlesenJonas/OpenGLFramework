#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/Misc/GPUTimer.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/TriangleTemplate.h>

#include <glad/glad/glad.h>
#include <glm/glm.hpp>

/*
  based on: https://onrendering.com/data/papers/cbt/ConcurrentBinaryTrees.pdf
  Concurrent Binary Trees (with application to longest edge bisection)
    by Jonathan Dupuy
*/

class Terrain;

class CBTGPU
{
  public:
    explicit CBTGPU(Terrain& terrain, uint32_t maxDepth, Texture& sceneDepthBuffer);
    ~CBTGPU();

    void update(const glm::mat4& projView, const glm::vec2 screenRes);
    void setTargetEdgeLength(float newLength);

    void doSumReduction();
    void writeIndirectCommands();

    void draw(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, Framebuffer& framebufferToWriteInto);
    void drawDepthOnly(const glm::mat4& projViewMatrix);
    void drawOutline(const glm::mat4& projViewMatrix);
    void drawOverlay(float aspect);
    void drawUI();
    void drawTimers();

    void setTemplateLevel(int newLevel);
    [[nodiscard]] int getTemplateLevel() const;
    [[nodiscard]] constexpr int getMaxTemplateLevel() const
    {
        return triangleTemplates.size() - 1;
    }
    struct Settings; // forward declare
    [[nodiscard]] inline const Settings getSettings() const
    {
        return settings;
    };

    // helpers for tests
    void replaceHeap(const std::vector<uint32_t>& heapData);
    std::vector<uint32_t> getHeap();
    uint32_t getAmountOfLeafNodes();

  private:
    Terrain& terrain;

    uint32_t maxDepth = 0xFFFFFFFF;
    GLuint cbtBuffer = 0xFFFFFFFF;
    uint64_t heapSizeInUint32s = 0;
    GLuint indirectDispatchCommandBuffer = 0xFFFFFFFF;
    GLuint indirectDrawCommandBuffer = 0xFFFFFFFF;

    ShaderProgram updateSplitShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/update.comp"}, {{"PASS", "SPLIT"}}};
    ShaderProgram updateMergeShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/update.comp"}, {{"PASS", "MERGE"}}};
    ShaderProgram sumReductionPassShader{COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/sumReduction.comp"}};
    ShaderProgram sumReductionLastDepthsShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/sumReductionLastDepths.comp"}};
    ShaderProgram writeIndirectCommandsShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/writeIndirectCommands.comp"}};

    std::array<TriangleTemplate, 8> triangleTemplates = {
        TriangleTemplate{0},
        TriangleTemplate{1},
        TriangleTemplate{2},
        TriangleTemplate{3},
        TriangleTemplate{4},
        TriangleTemplate{5},
        TriangleTemplate{6},
        TriangleTemplate{7}};
    FullscreenTri fullScreenTri;

    ShaderProgram drawDepthOnlyShader{
        VERTEX_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/drawing.vert"}, {{"DEPTH_ONLY_PASS", "1"}}};

    ShaderProgram outlineShader{
        VERTEX_SHADER_BIT | GEOMETRY_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/drawing.vert",
         SHADERS_PATH "/Terrain/CBT/outline.geom",
         SHADERS_PATH "/Terrain/CBT/outline.frag"}};
    ShaderProgram overlayShader{
        VERTEX_SHADER_BIT | GEOMETRY_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/overlay.vert",
         SHADERS_PATH "/Terrain/CBT/overlay.geom",
         SHADERS_PATH "/Terrain/CBT/overlay.frag"}};

    ShaderProgram drawShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/drawing.vert", SHADERS_PATH "/Terrain/CBT/drawing.frag"}};

    ShaderProgram drawUVBufferShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/uvbuffer.vert", SHADERS_PATH "/Terrain/CBT/uvbuffer.frag"}};
    ShaderProgram uvbufferScreenPassShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Terrain/CBT/uvbufferScreenPass.frag"}};
    ShaderProgram pixelCountingShader{COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/PixelCounting.comp"}};
    ShaderProgram pixelCountingWithCacheShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/PixelCountingWithCache.comp"}};
    GLuint pixelBufferSSBO;
    static constexpr int SHADING_GROUP_SIZE = 64;
    ShaderProgram pixelCountPrefixSumShader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/PixelCountPrefixSum.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}, {"RESET_AMOUNT_COUNTER", "1"}}};
    ShaderProgram pixelCountPrefixSumForCacheShader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/PixelCountPrefixSum.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram pixelSortingShader{COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/PixelSorting.comp"}};
    ShaderProgram pixelSortingFromCacheShader{
        COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/PixelSortingFromCache.comp"}};
    ShaderProgram renderUVBufferGroup0Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup0.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup1Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup1.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup2Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup2.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup3Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup3.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup4Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup4.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup5Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup5.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    ShaderProgram renderUVBufferGroup6Shader{
        COMPUTE_SHADER_BIT,
        {SHADERS_PATH "/Terrain/CBT/RenderPixelGroups/RenderUVBufferGroup6.comp"},
        {{"SHADING_GROUP_SIZE", std::to_string(SHADING_GROUP_SIZE)}}};
    Texture uvbufferRT;
    Texture posRT;
    Texture pixelIndexCache;
    Framebuffer uvbufferFramebuffer;
    Texture& sceneDepth;

    GPUTimer<128> mergeTimer{"Merge"};
    GPUTimer<128> splitTimer{"Split"};
    GPUTimer<128> sumReductionTimer{"Sum reduction"};
    GPUTimer<128> indirectWriteTimer{"Indirect command write"};
    GPUTimer<128> drawTimer{"Draw"};
    GPUTimer<128> uvBufferPassTimer{"uvBufferPass"};
    GPUTimer<128> pixelGroupingPassTimer{"pixelGroupingPass"};
    GPUTimer<128> pixelCountingTimer{"pixelCounting"};
    GPUTimer<128> pixelPrefixSumTimer{"pixelPrefixSum"};
    GPUTimer<128> pixelPosWriteTimer{"pixelPosWrite"};
    GPUTimer<128> shadingPassTimer{"shadingPass"};
    std::array<GPUTimer<128>, 7> shadingGroupTimers;

  public:
/* clang-format off */
#define TIMER_RETURN [[nodiscard]] inline const GPUTimer<128>&
    TIMER_RETURN getMergeTimer() const {return mergeTimer;}
    TIMER_RETURN getSplitTimer() const {return splitTimer;}
    TIMER_RETURN getSumReductionTimer() const {return sumReductionTimer;}
    TIMER_RETURN getIndirectWriteTimer() const {return indirectWriteTimer;}
    TIMER_RETURN getDrawTimer() const {return drawTimer;}
    /* clang-format on */

    struct Settings
    {
        int renderingMode = 0;
        bool drawOutline = false;
        bool freezeUpdates = false;
        int selectedSubdivLevel = 0;
    } settings;

    struct DrawElementsIndirectCommand
    {
        uint32_t count;
        uint32_t primCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t baseInstance;
    };
    struct DispatchIndirectCommand
    {
        uint32_t num_groups_x = 1;
        uint32_t num_groups_y = 1;
        uint32_t num_groups_z = 1;
    };
};