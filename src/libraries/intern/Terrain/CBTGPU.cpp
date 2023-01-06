#include "Terrain/CBTGPU.h"
#include "Misc/Misc.h"
#include "ShaderProgram/ShaderProgram.h"
#include "TriangleTemplate.h"

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
        // set bitheap[2^D] (converted to index in packed uint array) to 1, creating a single leaf node
        heap[(3 * (1u << maxDepth)) / 32] = 1u;
        assert(glm::findLSB(heap[0]) == maxDepth);

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
        // TODO: need to read amount of triangles in current selcted templateMesh
        const DrawElementsIndirectCommand temp{
            .count = triangleTemplates[selectedLevel].getIndexCount(),
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
        glUniform1ui(0, triangleTemplates[selectedLevel].getIndexCount());
    }

    doSumReduction();
    writeIndirectCommands();
}

CBTGPU::~CBTGPU()
{
    glDeleteBuffers(1, &cbtBuffer);
    glDeleteBuffers(1, &indirectDispatchCommandBuffer);
    glDeleteBuffers(1, &indirectDrawCommandBuffer);
}

void CBTGPU::update(glm::vec2 point)
{
    static bool splitPass = true;
    if(splitPass)
    {
        splitTimer.start();
        updateSplitShader.useProgram();
        glUniform2fv(0, 1, glm::value_ptr(point));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        splitTimer.end();
        splitTimer.evaluate();
    }
    else
    {
        mergeTimer.start();
        updateMergeShader.useProgram();
        glUniform2fv(0, 1, glm::value_ptr(point));
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        mergeTimer.end();
        mergeTimer.evaluate();
    }
    splitPass = !splitPass;
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
    sumReductionTimer.start();
    sumReductionPassShader.useProgram();
    // looping until >=0 doesnt work here since level is a uint, so its always >=0
    for(uint32_t level = maxDepth - 1; level < maxDepth; level--)
    {
        glUniform1ui(0, level);
        const uint32_t nodesAtDepth = 1u << level;
        glDispatchCompute(UintDivAndCeil(nodesAtDepth, 256), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    sumReductionTimer.end();
    sumReductionTimer.evaluate();
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
    glBindVertexArray(triangleTemplates[selectedLevel].getVAO());

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
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    outlineShader.useProgram();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(triangleTemplates[selectedLevel].getVAO());

    // glDrawElementsInstancedBaseVertexBaseInstance(
    // GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, leafNodeAmnt, 0, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void CBTGPU::drawOverlay(float aspect)
{
    overlayShader.useProgram();
    glUniform1f(0, aspect);
    glBindVertexArray(triangleTemplates[selectedLevel].getVAO());

    // glDrawElementsInstancedBaseVertexBaseInstance(
    // GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, leafNodeAmnt, 0, 0);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
}

void CBTGPU::setTemplateLevel(int newLevel)
{
    if(newLevel < 0 || newLevel >= triangleTemplates.size())
        return;
    selectedLevel = newLevel;
    writeIndirectCommandsShader.useProgram();
    glUniform1ui(0, triangleTemplates[selectedLevel].getIndexCount());
}

[[nodiscard]] int CBTGPU::getTemplateLevel() const
{
    return selectedLevel;
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