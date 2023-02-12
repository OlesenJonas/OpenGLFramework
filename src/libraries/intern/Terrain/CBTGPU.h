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
    /* specific update function for tests replicating the CPU version */
    void refineAroundPoint(glm::vec2 point);

    void doSumReduction();
    void writeIndirectCommands();

    void draw(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, Framebuffer& framebufferToWriteInto);
    void drawDepthOnly(const glm::mat4& projViewMatrix);
    void drawOutline(const glm::mat4& projViewMatrix);
    void drawOverlay(float aspect);
    void drawUI();

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

    ShaderProgram updateSplitShader;
    ShaderProgram updateMergeShader;
    ShaderProgram refineAroundPointSplitShader;
    ShaderProgram refineAroundPointMergeShader;
    ShaderProgram sumReductionPassShader;
    ShaderProgram sumReductionLastDepthsShader;
    ShaderProgram writeIndirectCommandsShader;

    std::array<TriangleTemplate, 8> triangleTemplates = {
        TriangleTemplate{0},
        TriangleTemplate{1},
        TriangleTemplate{2},
        TriangleTemplate{3},
        TriangleTemplate{4},
        TriangleTemplate{5},
        TriangleTemplate{6},
        TriangleTemplate{7}};
    ShaderProgram drawDepthOnlyShader;
    ShaderProgram drawShader;
    ShaderProgram drawVisBufferShader;
    FullscreenTri fullScreenTri;
    ShaderProgram visbufferScreenPassShader;
    ShaderProgram pixelCountingShader;
    GLuint pixelBufferSSBO;
    ShaderProgram pixelCountPrefixSumShader;
    ShaderProgram pixelSortingShader;
    ShaderProgram renderUVBufferGroup0Shader;
    ShaderProgram renderUVBufferGroup1Shader;
    ShaderProgram renderUVBufferGroup2Shader;
    ShaderProgram renderUVBufferGroup3Shader;
    ShaderProgram renderUVBufferGroup4Shader;
    ShaderProgram renderUVBufferGroup5Shader;
    ShaderProgram renderUVBufferGroup6Shader;
    Texture visbufferTarget;
    Texture posTarget;
    Framebuffer visbufferFramebuffer;
    Texture& sceneDepth;
    ShaderProgram outlineShader;
    ShaderProgram overlayShader;

    GPUTimer<128> mergeTimer{"Merge"};
    GPUTimer<128> splitTimer{"Split"};
    GPUTimer<128> sumReductionTimer{"Sum reduction"};
    GPUTimer<128> indirectWriteTimer{"Indirect command write"};
    GPUTimer<128> drawTimer{"Draw"};

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