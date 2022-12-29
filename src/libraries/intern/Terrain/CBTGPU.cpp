#include "CBTGPU.h"
#include "ShaderProgram/ShaderProgram.h"
#include "Terrain/TriangleTemplate.h"

#include <array>
#include <glm/integer.hpp>

CBTGPU::CBTGPU(uint32_t maxDepth)
    : maxDepth(maxDepth),                                                             //
      cbtUpdateShader(COMPUTE_SHADER_BIT, {SHADERS_PATH "/Terrain/CBT/Update.comp"}), //
      triangleMesh(0),                                                                //
      drawShader(
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/CBT/drawing.vert", SHADERS_PATH "/Terrain/CBT/drawing.frag"})
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
        cbtBufferByteSize = bitsNeeded / 8;
        // for all realistic scenarios 2^(D+2) will be cleanly divisible by 32 = 2^5
        const uint64_t uint32Needed = bitsNeeded / 32u;

        heap.resize(uint32Needed);
        std::fill(heap.begin(), heap.end(), 0u);

        // store max depth in first D+3 bits
        heap[0] = 1u << maxDepth;
        // set bitheap[2^D] (converted to index in packed uint array) to 1, creating a single leaf node
        heap[(3 * (1u << maxDepth)) / 32] = 1u;
        assert(glm::findLSB(heap[0]) == maxDepth);

        glCreateBuffers(1, &cbtBuffer);
        glNamedBufferStorage(cbtBuffer, heap.size() * sizeof(heap[0]), &heap, 0);
        glObjectLabel(GL_BUFFER, cbtBuffer, -1, "CBT Buffer");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cbtBuffer);
    }

    {
        typedef struct
        {
            uint32_t num_groups_x;
            uint32_t num_groups_y;
            uint32_t num_groups_z;
        } DispatchIndirectCommand;
        const DispatchIndirectCommand temp{1, 1, 1};

        glCreateBuffers(1, &indirectDispatchCommandBuffer);
        glNamedBufferStorage(indirectDispatchCommandBuffer, sizeof(DispatchIndirectCommand), &temp, 0);
        glObjectLabel(GL_BUFFER, indirectDispatchCommandBuffer, -1, "Indirect Dispatch Command Buffer");
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirectDispatchCommandBuffer);
    }

    {
        // TODO: need to read amount of triangles in current selcted templateMesh
        typedef struct
        {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t first;
            uint32_t baseInstance;
        } DrawArraysIndirectCommand;
        const DrawArraysIndirectCommand temp{1, 1, 0, 0};

        // TODO: switch. using index buffer, so this indirect command strcture is needed
        typedef struct
        {
            uint32_t count;
            uint32_t primCount;
            uint32_t firstIndex;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } DrawElementsIndirectCommand;

        glCreateBuffers(1, &indirectDrawCommandBuffer);
        glNamedBufferStorage(indirectDrawCommandBuffer, sizeof(DrawArraysIndirectCommand), &temp, 0);
        glObjectLabel(GL_BUFFER, indirectDispatchCommandBuffer, -1, "Indirect Draw Command Buffer");
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectDispatchCommandBuffer);
    }

    // doSumReduction();

    // {
    //     glCreateVertexArrays(1, &vaoHandle);
    //     glBindVertexArray(vaoHandle);

    //     glGenBuffers(1, &vboHandle);

    //     glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
    //     // allocate space for 3 corners of type vec2 for the max number of possible leaf nodes
    //     // the max number of leaf nodes possible exists if every single node is split
    //     // the amount of leaf nodes is then equal to the amount of nodes on the last level
    //     const uint32_t maxNumberOfLeafNodes = 1U << maxDepth;
    //     glBufferStorage(
    //         GL_ARRAY_BUFFER, sizeof(glm::vec2) * 3 * maxNumberOfLeafNodes, nullptr,
    //         GL_DYNAMIC_STORAGE_BIT);

    //     // position
    //     glEnableVertexAttribArray(0);
    //     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

    //     glBindBuffer(GL_ARRAY_BUFFER, 0);
    // }

    // updateDrawData();
}

void CBTGPU::update(glm::vec2 point)
{
}

void CBTGPU::draw(const glm::mat4& projViewMatrix)
{
    drawShader.useProgram();
    // a lot of this will simplify to indirect calls later
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(triangleMesh.getVAO());

    // const uint32_t leafNodeAmnt = getNodeValue({1, 0});
    const uint32_t leafNodeAmnt = 4;

    glUniform1i(1, 0);
    glDrawElementsInstancedBaseVertexBaseInstance(
        GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, 2, 0, 0);
}

void CBTGPU::drawOutline(const glm::mat4& projViewMatrix)
{
    drawShader.useProgram();
    // a lot of this will simplify to indirect calls later
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(triangleMesh.getVAO());

    // const uint32_t leafNodeAmnt = getNodeValue({1, 0});
    const uint32_t leafNodeAmnt = 4;

    // render triangle outlines with hidden line removal
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform1i(1, 1);
    glPolygonOffset(-1.0, 1.0);
    glDrawElementsInstancedBaseVertexBaseInstance(
        GL_TRIANGLES, triangleMesh.getIndexCount(), GL_UNSIGNED_INT, nullptr, 2, 0, 0);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}