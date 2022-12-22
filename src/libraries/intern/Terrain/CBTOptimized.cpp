#include "CBTOptimized.h"

#include <array>
#include <glm/integer.hpp>

CBTOptimized::CBTOptimized(uint32_t maxDepth)
    : shader(
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/triangleTreeVis.vert", SHADERS_PATH "/Terrain/triangleTreeVis.frag"})
{
    // im not actually sure what the max possible depth is when using uint32_ts for the bitheap
    // would need to check code, but I think 30 should be fine. That way 1 << (30+1) can still be represented
    assert(maxDepth < 30);
    assert(maxDepth >= 5);

    // the number of bits needed to store all bits neded for heap (see paper)
    const uint64_t bitsNeeded = 1ull << (maxDepth + 2u);
    // for all realistic scenarios 2^(D+2) will be cleanly divisible by 32 = 2^5
    const uint64_t uint32Needed = bitsNeeded / 32u;

    heap.resize(uint32Needed);
    std::fill(heap.begin(), heap.end(), 0u);

    // store max depth in first D+3 bits
    heap[0] = 1u << maxDepth;
    setNodeBitInBitfield({1, 0}, 1u);
    assert(getNodeBitInBitfield({1, 0}) == 1u);
    // set bitheap[2^D] (converted to index in packed uint array) to 1, creating a single leaf node
    assert(heap[3 * (1u << maxDepth) / 32] == 1u);
    assert(glm::findLSB(heap[0]) == maxDepth);

    doSumReduction();

    {
        glCreateVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glGenBuffers(1, &vboHandle);

        glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
        // allocate space for 3 corners of type vec2 for the max number of possible leaf nodes
        // the max number of leaf nodes possible exists if every single node is split
        // the amount of leaf nodes is then equal to the amount of nodes on the last level
        const uint32_t maxNumberOfLeafNodes = 1U << maxDepth;
        glBufferStorage(
            GL_ARRAY_BUFFER, sizeof(glm::vec2) * 3 * maxNumberOfLeafNodes, nullptr, GL_DYNAMIC_STORAGE_BIT);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    updateDrawData();
}

CBTOptimized::Corners CBTOptimized::cornersOfLeftChild(const Corners& corners)
{
#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    return {corners[2], corners[0], 0.5f * (corners[0] + corners[1])};
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    return {corners[0], 0.5f * (corners[0] + corners[2]), corners[1]};
#endif
}

CBTOptimized::Corners CBTOptimized::cornersOfRightChild(const Corners& corners)
{
#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    return {corners[1], corners[2], 0.5f * (corners[0] + corners[1])};
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    return {corners[1], 0.5f * (corners[0] + corners[2]), corners[2]};
#endif
}

bool CBTOptimized::pointInTriangle(glm::vec2 pos, const std::array<glm::vec2, 3>& corners)
{
    /* unoptimized but readable */

    const glm::vec2 edge0 = corners[1] - corners[0];
    const glm::vec2 nrm0{-edge0.y, edge0.x};
    const bool inside1 = glm::dot(pos - corners[0], nrm0) < 0.0;
    // if(glm::dot(pos - corners[0], nrm0) > 0.0)
    // return false;

    const glm::vec2 edge1 = corners[2] - corners[1];
    const glm::vec2 nrm1{-edge1.y, edge1.x};
    const bool inside2 = glm::dot(pos - corners[1], nrm1) < 0.0;
    // if(glm::dot(pos - corners[1], nrm1) > 0.0)
    // return false;

    const glm::vec2 edge2 = corners[0] - corners[2];
    const glm::vec2 nrm2{-edge2.y, edge2.x};
    const bool inside3 = glm::dot(pos - corners[2], nrm2) < 0.0;
    // if(glm::dot(pos - corners[2], nrm2) > 0.0)
    // return false;

    // this test works for triangles of either winding order
    return (inside1 == inside2 && inside2 == inside3);
    // return true;
}

std::array<glm::vec2, 3> CBTOptimized::cornersFromNode(Node node)
{
    std::array<glm::vec2, 3> corners{p0, p1, p2};
    for(int bitID = node.depth - 1; bitID >= 0; bitID--)
    {
        if(getSingleBitValue(node.heapIndex, bitID) == 0u)
        {
            corners = cornersOfLeftChild(corners);
        }
        else
        {
            corners = cornersOfRightChild(corners);
        }
    }

    return corners;
}

void CBTOptimized::updateDrawData()
{
    const uint32_t leafNodeAmnt = getNodeValue({1, 0});
    std::vector<glm::vec2> vertexPositions(leafNodeAmnt * 3);

    // after sumreduction the 1st element contains the total number of leaf nodes
    for(uint32_t i = 0; i < leafNodeAmnt; i++)
    {
        const Node leafNode = leafIndexToNode(i);

        std::array<glm::vec2, 3> corners = cornersFromNode(leafNode);

#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
        if((leafNode.depth & 1u) != 0u)
        {
            // tri subdivision used in paper has the disadvantege of flipping the winding order every level
            corners = {corners[0], corners[2], corners[1]};
        }
#endif
        vertexPositions[i * 3 + 0] = corners[0];
        vertexPositions[i * 3 + 1] = corners[1];
        vertexPositions[i * 3 + 2] = corners[2];
    }

    glNamedBufferSubData(vboHandle, 0, leafNodeAmnt * 3 * sizeof(glm::vec2), vertexPositions.data());
}

void CBTOptimized::draw(const glm::mat4& projViewMatrix)
{
    shader.useProgram();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(vaoHandle);

    const uint32_t leafNodeAmnt = getNodeValue({1, 0});

    glUniform1i(1, 0);
    glDrawArrays(GL_TRIANGLES, 0, leafNodeAmnt * 3);

    // render triangle outlines with hidden line removal
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform1i(1, 1);
    glPolygonOffset(-1.0, 1.0);
    glDrawArrays(GL_TRIANGLES, 0, leafNodeAmnt * 3);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

CBTOptimized::Node CBTOptimized::nodeFromPoint(glm::vec2 p)
{
    std::array<glm::vec2, 3> corners{p0, p1, p2};
    if(!pointInTriangle(p, corners))
    {
        return {0, 0};
    }
    // point is inside 0th-level triangle, so its definitly inside one of the leaf triangles
    uint32_t currentHeapIndex = 1;
    uint32_t nodeDepth = 0;
    while(getNodeValue({currentHeapIndex, nodeDepth}) > 1)
    {
        const Corners leftChildCorners = cornersOfLeftChild(corners);
        if(pointInTriangle(p, leftChildCorners))
        {
            // inside left child triangle
            currentHeapIndex = 2 * currentHeapIndex;
            corners = leftChildCorners;
        }
        else
        {
            // else must be inside right child triangle
            currentHeapIndex = 2 * currentHeapIndex + 1;
            corners = cornersOfRightChild(corners);
        }
        nodeDepth++;
        assert(nodeDepth == glm::findMSB(currentHeapIndex));
    }
    return {currentHeapIndex, nodeDepth};
}