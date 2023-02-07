#include "CBTGPU.h"
#include "Misc/Misc.h"
#include "ShaderProgram/ShaderProgram.h"
#include "TriangleTemplate.h"

#include <ImGui/imgui.h>

#include <array>

CBTGPU::CBTGPU(uint32_t maxDepth)
    : maxDepth(maxDepth), //
      refineAroundPointSplitShader(
          COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/refineAroundPoint.comp"}, {{"PASS", "SPLIT"}}),
      refineAroundPointMergeShader(
          COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/refineAroundPoint.comp"}, {{"PASS", "MERGE"}}),
      updateSplitShader(COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/update.comp"}, {{"PASS", "SPLIT"}}),
      updateMergeShader(COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/update.comp"}, {{"PASS", "MERGE"}}),
      sumReductionPassShader(COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/sumReduction.comp"}),
      sumReductionLastDepthsShader(
          COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/sumReductionLastDepths.comp"}),
      writeIndirectCommandsShader(
          COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/writeIndirectCommands.comp"}),
      drawShader(
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/CBT/drawing.vert", SHADERS_PATH "/Terrain/CBT/drawing.frag"}),
      outlineShader(
          VERTEX_SHADER_BIT | GEOMETRY_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/CBT/outline.vert",
           SHADERS_PATH "/Terrain/CBT/outline.geom",
           SHADERS_PATH "/Terrain/CBT/outline.frag"}),
      overlayShader(
          VERTEX_SHADER_BIT | GEOMETRY_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/CBT/overlay.vert",
           SHADERS_PATH "/Terrain/CBT/overlay.geom",
           SHADERS_PATH "/Terrain/CBT/overlay.frag"})
{
    // im not actually sure what the max possible depth is when using uint32_ts for the bitheap
    // would need to check code, but I think 30 should be fine. That way 1 << (30+1) can still be represented
    assert(maxDepth <= 30);
    assert(maxDepth >= 7);

    {
        // binary heap packed together
        std::vector<uint32_t> heap;
        // the number of bits needed to store all bits neded for heap (see paper)
        const uint64_t bitsNeeded = 1ull << (maxDepth + 2u);
        // for all realistic scenarios 2^(D+2) will be cleanly divisible by 32 = 2^5
        heapSizeInUint32s = bitsNeeded / 32u;

        heap.resize(heapSizeInUint32s);
        std::fill(heap.begin(), heap.end(), 0u);

        // store max depth in first D+3 bits
        heap[0] = 1u << maxDepth;
        assert(glm::findLSB(heap[0]) == maxDepth);

        uint32_t levelToInit = 2;
        {
            // from LowLevel.glsl
            auto setNodeBitInBitfield = [&maxDepth, &heap](uint32_t nodeHeapIndex, uint32_t nodeDepth)
            {
                const int depthOffsetToBitfieldPart = maxDepth - int(nodeDepth);
                uint32_t nodeAtBitfieldDepthHeapIndex = nodeHeapIndex << depthOffsetToBitfieldPart;
                nodeDepth = maxDepth;

                const uint32_t bitIndex =
                    (1u << (maxDepth + 1u)) +
                    nodeAtBitfieldDepthHeapIndex; // * nodeAtBitfieldDepth.depth (which is == 1 here);

                const uint32_t arrIndex = bitIndex / 32u;
                const uint32_t localBitIndex = bitIndex % 32u;

                const uint32_t mask = ~(1u << localBitIndex);
                // atomicAnd(heap[arrIndex], mask);
                heap[arrIndex] &= mask;
                // atomicOr(heap[arrIndex], value << localBitIndex);
                heap[arrIndex] |= (1u << localBitIndex);
            };

            const uint32_t levelStartIndex = 1u << levelToInit;
            const uint32_t amountAtLevel = 1u << levelToInit;
            for(uint32_t i = 0; i < amountAtLevel; i++)
            {
                const uint32_t realIndex = levelStartIndex + i;
                setNodeBitInBitfield(realIndex, levelToInit);
            }
        }
        // set bitheap[2^D] (converted to index in packed uint array) to 1, creating a single leaf node
        // heap[(3 * (1u << maxDepth)) / 32] = 1u;

        glCreateBuffers(1, &cbtBuffer);
        glNamedBufferStorage(cbtBuffer, heap.size() * sizeof(heap[0]), heap.data(), 0);
        glObjectLabel(GL_BUFFER, cbtBuffer, -1, "CBT Buffer");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cbtBuffer);
    }

    {
        const DispatchIndirectCommand temp{1, 1, 1};

        glCreateBuffers(1, &indirectDispatchCommandBuffer);
        glNamedBufferStorage(indirectDispatchCommandBuffer, sizeof(DispatchIndirectCommand), &temp, 0);
        glObjectLabel(GL_BUFFER, indirectDispatchCommandBuffer, -1, "Indirect Dispatch Command Buffer");
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirectDispatchCommandBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indirectDispatchCommandBuffer);
    }

    {
        const DrawElementsIndirectCommand temp{
            .count = triangleTemplates[settings.selectedSubdivLevel].getIndexCount(),
            .primCount = 1, // number of instances
            .firstIndex = 0,
            .baseVertex = 0,
            .baseInstance = 0};

        glCreateBuffers(1, &indirectDrawCommandBuffer);
        glNamedBufferStorage(indirectDrawCommandBuffer, sizeof(DrawElementsIndirectCommand), &temp, 0);
        glObjectLabel(GL_BUFFER, indirectDrawCommandBuffer, -1, "Indirect Draw Command Buffer");
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectDrawCommandBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indirectDrawCommandBuffer);

        writeIndirectCommandsShader.useProgram();
        glUniform1ui(0, triangleTemplates[settings.selectedSubdivLevel].getIndexCount());
    }

    doSumReduction();
    writeIndirectCommands();
    setTargetEdgeLength(20);
}

CBTGPU::~CBTGPU()
{
    glDeleteBuffers(1, &cbtBuffer);
    glDeleteBuffers(1, &indirectDispatchCommandBuffer);
    glDeleteBuffers(1, &indirectDrawCommandBuffer);
}

void CBTGPU::update(const glm::mat4& projView, const glm::vec2 screenRes)
{
    static bool splitPass = true;
    if(splitPass)
    {
        splitTimer.start();
        updateSplitShader.useProgram();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projView));
        glUniform2fv(1, 1, glm::value_ptr(screenRes));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        splitTimer.end();
        splitTimer.evaluate();
    }
    else
    {
        mergeTimer.start();
        updateMergeShader.useProgram();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projView));
        glUniform2fv(1, 1, glm::value_ptr(screenRes));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        mergeTimer.end();
        mergeTimer.evaluate();
    }
    splitPass = !splitPass;
}

void CBTGPU::setTargetEdgeLength(float newLength)
{
    updateMergeShader.useProgram();
    glUniform1f(2, newLength);
    updateSplitShader.useProgram();
    glUniform1f(2, newLength);
}

void CBTGPU::refineAroundPoint(glm::vec2 point)
{
    static bool splitPass = true;
    if(splitPass)
    {
        splitTimer.start();
        refineAroundPointSplitShader.useProgram();
        glUniform2fv(0, 1, glm::value_ptr(point));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        splitTimer.end();
        splitTimer.evaluate();
    }
    else
    {
        mergeTimer.start();
        refineAroundPointMergeShader.useProgram();
        glUniform2fv(0, 1, glm::value_ptr(point));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        mergeTimer.end();
        mergeTimer.evaluate();
    }
    splitPass = !splitPass;
}

void CBTGPU::doSumReduction()
{
#define USE_OPTIMIZED

    sumReductionTimer.start();

#ifdef USE_OPTIMIZED
    sumReductionLastDepthsShader.useProgram();
    // todo: really only need to set this uniform once in ctor!
    glUniform1ui(0, maxDepth - 5);
    const uint32_t nodesAtDepth = 1u << (maxDepth - 5);
    glDispatchCompute(UintDivAndCeil(nodesAtDepth, 256), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#endif

    sumReductionPassShader.useProgram();
    // looping until >=0 doesnt work here since level is a uint, so its always >=0
#ifdef USE_OPTIMIZED
    for(uint32_t level = maxDepth - 6; level < maxDepth; level--)
#else
    for(uint32_t level = maxDepth - 1; level < maxDepth; level--)
#endif
    {
        glUniform1ui(0, level);
        const uint32_t nodesAtDepth = 1u << level;
        glDispatchCompute(UintDivAndCeil(nodesAtDepth, 256), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    sumReductionTimer.end();
    sumReductionTimer.evaluate();
#undef USE_OPTIMIZED
}

void CBTGPU::writeIndirectCommands()
{
    indirectWriteTimer.start();
    writeIndirectCommandsShader.useProgram();
    glDispatchCompute(1, 1, 1);
    // spec says that command_barrier is for draw*indirect, but nothing about dispatchIndirect ?!
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    // glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    indirectWriteTimer.end();
    indirectWriteTimer.evaluate();
}

void CBTGPU::draw(const glm::mat4& projViewMatrix)
{
    drawTimer.start();
    drawShader.useProgram();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(triangleTemplates[settings.selectedSubdivLevel].getVAO());

    // glDrawElementsInstancedBaseVertexBaseInstance(
    // GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, leafNodeAmnt, 0, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    drawTimer.end();
    drawTimer.evaluate();
}

void CBTGPU::drawOutline(const glm::mat4& projViewMatrix)
{
    // with the current depth range (0.01 to 1000) using just a depth offset
    // doesnt work anymore. Either its too small or too large
    // so just render without depth test and do masking in fragment shader
    // glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    outlineShader.useProgram();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(triangleTemplates[settings.selectedSubdivLevel].getVAO());

    // glDrawElementsInstancedBaseVertexBaseInstance(
    // GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, leafNodeAmnt, 0, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    // glEnable(GL_DEPTH_TEST);
}

void CBTGPU::drawOverlay(float aspect)
{
    overlayShader.useProgram();
    glUniform1f(0, aspect);
    glBindVertexArray(triangleTemplates[settings.selectedSubdivLevel].getVAO());

    // glDrawElementsInstancedBaseVertexBaseInstance(
    // GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, leafNodeAmnt, 0, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
}

void CBTGPU::drawUI()
{
    static float targetEdgeLength = 7.0f;
    if(ImGui::SliderFloat("Target edge length", &targetEdgeLength, 1.0f, 100.0f))
    {
        setTargetEdgeLength(targetEdgeLength);
    }
    static int globalSubdivLevel = 0;
    if(ImGui::SliderInt("Global subdiv level", &globalSubdivLevel, 0, getMaxTemplateLevel()))
    {
        setTemplateLevel(globalSubdivLevel);
    }
    ImGui::Checkbox("Draw outline", &settings.drawOutline);
    ImGui::Checkbox("Freeze update", &settings.freezeUpdates);
    ImGui::Separator();
    if(ImGui::DragFloat("Triplanar blending sharpness", &settings.triplanarSharpness, 0.05f, 0.0f, 10.0f))
    {
        drawShader.useProgram();
        glUniform1f(
            glGetUniformLocation(drawShader.getProgramID(), "triplanarSharpness"),
            settings.triplanarSharpness);
    }
    if(ImGui::SliderFloat("Material normal intensity", &settings.materialNormalIntensity, 0.0f, 1.0f))
    {
        drawShader.useProgram();
        glUniform1f(3, settings.materialNormalIntensity);
    }
    if(ImGui::SliderFloat(
           "Material displacement intensity", &settings.materialDisplacementIntensity, 0.0f, 1.0f))
    {
        drawShader.useProgram();
        glUniform1f(1, settings.materialDisplacementIntensity);
    }
    if(ImGui::SliderInt("Material displacement lod offset", &settings.materialDisplacementLodOffset, 0, 7))
    {
        drawShader.useProgram();
        glUniform1i(2, settings.materialDisplacementLodOffset);
    }
}

void CBTGPU::setTemplateLevel(int newLevel)
{
    if(newLevel < 0 || newLevel >= triangleTemplates.size())
        return;
    settings.selectedSubdivLevel = newLevel;
    writeIndirectCommandsShader.useProgram();
    glUniform1ui(0, triangleTemplates[settings.selectedSubdivLevel].getIndexCount());
}

[[nodiscard]] int CBTGPU::getTemplateLevel() const
{
    return settings.selectedSubdivLevel;
}

void CBTGPU::replaceHeap(const std::vector<uint32_t>& heapData)
{
    assert(heapData.size() == heapSizeInUint32s);

    GLuint uploadBuffer = 0xFFFFFFFF;
    glCreateBuffers(1, &uploadBuffer);
    glNamedBufferStorage(uploadBuffer, heapData.size() * sizeof(heapData[0]), heapData.data(), 0);
    glCopyNamedBufferSubData(uploadBuffer, cbtBuffer, 0, 0, heapSizeInUint32s * 4);
    glDeleteBuffers(1, &uploadBuffer);
}

std::vector<uint32_t> CBTGPU::getHeap()
{
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    std::vector<uint32_t> heap(heapSizeInUint32s);
    glGetNamedBufferSubData(cbtBuffer, 0, heapSizeInUint32s * 4, heap.data());

    return heap;
}

uint32_t CBTGPU::getAmountOfLeafNodes()
{
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    DrawElementsIndirectCommand drawCmd;
    DispatchIndirectCommand dispatchCmd;

    glGetNamedBufferSubData(indirectDrawCommandBuffer, 0, sizeof(drawCmd), &drawCmd);
    glGetNamedBufferSubData(indirectDispatchCommandBuffer, 0, sizeof(dispatchCmd), &dispatchCmd);

    assert(UintDivAndCeil(drawCmd.primCount, 256) == dispatchCmd.num_groups_x);
    assert(dispatchCmd.num_groups_y == 1 && dispatchCmd.num_groups_z == 1);

    return drawCmd.primCount;
}