#include "CBT.h"

#include <array>
#include <glm/integer.hpp>

CBT::CBT(uint32_t maxDepth)
    : maxDepth(maxDepth),
      shader(
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/Terrain/triangleTreeVis.vert", SHADERS_PATH "/Terrain/triangleTreeVis.frag"})
{
    // allocate 2^(D+1) nodes
    heap.resize(1u << (maxDepth + 1));
    std::fill(heap.begin(), heap.end(), 0);
    // set heap[2^D] to 1, creating a single leaf node
    heap[1u << maxDepth] = 1;

    doSumReduction();

    const uint32_t maxNumberOfLeafNodes = 1U << maxDepth;
    {
        glCreateVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glGenBuffers(1, &vboHandle);

        glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
        // allocate space for 3 corners of type vec2 for the max number of possible leaf nodes
        glBufferStorage(
            GL_ARRAY_BUFFER, sizeof(glm::vec2) * 3 * maxNumberOfLeafNodes, nullptr, GL_DYNAMIC_STORAGE_BIT);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    updateDrawData();
}

uint32_t CBT::getSingleBitValue(uint32_t field, int bitIndex)
{
    return ((field >> bitIndex) & 1u);
}

CBT::Corners CBT::cornersOfLeftChild(const Corners& corners)
{
#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    return {corners[2], corners[0], 0.5f * (corners[0] + corners[1])};
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    return {corners[0], 0.5f * (corners[0] + corners[2]), corners[1]};
#endif
}

CBT::Corners CBT::cornersOfRightChild(const Corners& corners)
{
#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    return {corners[1], corners[2], 0.5f * (corners[0] + corners[1])};
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    return {corners[1], 0.5f * (corners[0] + corners[2]), corners[2]};
#endif
}

bool CBT::pointInTriangle(glm::vec2 pos, const std::array<glm::vec2, 3>& corners)
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

/*
    this test is not 100% correct since a node and its left child
    both have the same index in the bitfield.
    That means that a "1" in the bitfield only indicates that either
    the node itself OR any of its left children are a leaf node!
*/
bool CBT::isLeafNode(Node node)
{
    const int depthOffsetToBitfieldPart = maxDepth - node.depth;
    const uint32_t heapIndexOfBitfieldPosition = node.heapIndex << depthOffsetToBitfieldPart;
    return heap[heapIndexOfBitfieldPosition] == 1;
}

bool CBT::isParentOfTwoLeafNodes(Node node)
{
    return heap[node.heapIndex] <= 2;
}

void CBT::splitNode(Node node)
{
    const int depthOffsetToBitfieldPart = maxDepth - node.depth;

    // split
    // this is only called from splitNodeConforming() atm which already checks this
    // maybe useful to have a splitNode() and a splitNodeUnsafe() function
    // if(nodeDepth < maxDepth)
    // {
    const uint32_t rightChildIndex = (2 * node.heapIndex) | 1U;
    const uint32_t rightChildIndexInBitfield = rightChildIndex << (depthOffsetToBitfieldPart - 1u);
    heap[rightChildIndexInBitfield] = 1;
    // }
}

void CBT::splitNodeConforming(Node node)
{
    if(node.depth < maxDepth)
    {
        splitNode(node);
        const uint32_t edgeNeighbourID = calculateSameDepthNeighbourhood(node).edge;
        Node currentNode{edgeNeighbourID, edgeNeighbourID == 0 ? 0 : node.depth};

        while(currentNode.heapIndex > 1)
        {
            splitNode(currentNode);
            // factor out into getParent() ?
            currentNode.heapIndex = currentNode.heapIndex / 2;
            currentNode.depth -= 1;
            splitNode(currentNode);
            const uint32_t edgeNeighbourID = calculateSameDepthNeighbourhood(currentNode).edge;
            currentNode = {edgeNeighbourID, edgeNeighbourID == 0 ? 0 : currentNode.depth};
        }
    }
}

void CBT::mergeNode(Node node)
{
    const int depthOffsetToBitfieldPart = maxDepth - node.depth;

    // merge
    if(node.depth > 0)
    {
        const uint32_t rightSiblingIndex = node.heapIndex | 1U;
        const uint32_t rightSiblingIndexInBitfield = rightSiblingIndex << depthOffsetToBitfieldPart;
        heap[rightSiblingIndexInBitfield] = 0;
    }
}

void CBT::mergeNodeConforming(Node node)
{
    // cant merge root node
    if(node.heapIndex <= 1)
    {
        return;
    }

    /*
        slightly different logic than outliend in paper
        (mostly because isLeafNode doesnt work as intended, see function comment)
    */
    const Node parentNode{node.heapIndex / 2, node.depth - 1};
    const bool canSplitTop = isParentOfTwoLeafNodes(parentNode);

    const uint32_t diamondIndex = calculateSameDepthNeighbourhood(parentNode).edge;
    // set diamond index = parentIndex if diamondIndex doesnt exist, saves some braching later on
    const Node diamondNode{diamondIndex == 0 ? parentNode.heapIndex : diamondIndex, parentNode.depth};
    const bool canSplitBot = isParentOfTwoLeafNodes(diamondNode);

    if(canSplitTop && canSplitBot)
    {
        mergeNode(node);
        const Node rightDiamondChild{2 * diamondNode.heapIndex + 1, diamondNode.depth + 1};
        mergeNode(rightDiamondChild);
    }
}

CBT::SameDepthNeighbourhood
CBT::neighbourhoodAfterSplit(SameDepthNeighbourhood neighbourhood, uint32_t direction) // NOLINT
{
    // 0 represents "null" and null+1 needs to stay 0 / null
    const uint32_t summandLeft = (neighbourhood.left == 0u) ? 0u : 1u;
    const uint32_t summandRight = (neighbourhood.right == 0u) ? 0u : 1u;
    const uint32_t summandEdge = (neighbourhood.edge == 0u) ? 0u : 1u;

#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    if(direction == 0u)
    {
        return {
            2 * neighbourhood.self + 1, // self is always != null
            2 * neighbourhood.edge + summandEdge,
            2 * neighbourhood.left + summandLeft,
            2 * neighbourhood.self};
    }
    if(direction == 1u)
    {
        return {
            2 * neighbourhood.edge,
            2 * neighbourhood.self,
            2 * neighbourhood.right,
            2 * neighbourhood.self + 1};
    }
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    if(direction == 0u)
    {
        return {
            2 * neighbourhood.self + 1, // self is always != null
            2 * neighbourhood.edge + summandEdge,
            2 * neighbourhood.right + summandRight,
            2 * neighbourhood.self};
    }
    if(direction == 1u)
    {
        return {
            2 * neighbourhood.edge,
            2 * neighbourhood.self,
            2 * neighbourhood.left,
            2 * neighbourhood.self + 1};
    }
#endif
    assert(false && "should be unreachable");
    return {};
}

CBT::SameDepthNeighbourhood CBT::calculateSameDepthNeighbourhood(Node node)
{
    SameDepthNeighbourhood neighbourhood{0u, 0u, 0u, 1u};

    for(int bitID = node.depth - 1; bitID >= 0; bitID--)
    {
        neighbourhood = neighbourhoodAfterSplit(neighbourhood, getSingleBitValue(node.heapIndex, bitID));
    }
    assert(node.heapIndex == 0 || node.heapIndex == neighbourhood.self);

    return neighbourhood;
}

CBT::Node CBT::leafIndexToNode(uint32_t leafIndex)
{
    uint32_t currentHeapIndex = 1;
    uint32_t nodeDepth = 0;
    while(heap[currentHeapIndex] > 1)
    {
        const uint32_t leftChildHeapIndex = currentHeapIndex * 2;
        const uint32_t rightChildHeapIndex = currentHeapIndex * 2 + 1;
        if(leafIndex < heap[leftChildHeapIndex])
        {
            currentHeapIndex = leftChildHeapIndex;
        }
        else
        {
            leafIndex -= heap[leftChildHeapIndex];
            currentHeapIndex = rightChildHeapIndex;
        }
        nodeDepth++;
    }
    assert(nodeDepth == glm::findMSB(currentHeapIndex));

    return {currentHeapIndex, nodeDepth};
}

std::array<glm::vec2, 3> CBT::cornersFromNode(Node node)
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

void CBT::refineAroundPoint(glm::vec2 p)
{
    static bool splitPass = true;

    // after sumreduction the 1st element contains the total number of leaf nodes
    for(uint32_t i = 0; i < heap[1]; i++)
    {
        const Node leafNode = leafIndexToNode(i);
        /*
            writing to the heap can be done atomically
            but the CBT still cant handle cases where the same
            node is split and merged (by a different mode) in
            the same iteration, so alternate
        */
        if(splitPass)
        {
            const std::array<glm::vec2, 3> corners = cornersFromNode(leafNode);
            if(pointInTriangle(p, corners))
            {
                splitNodeConforming(leafNode);
            }
        }
        else
        {
            /*
                make sure both diamond parents fulfill the requirements for a merge
                otherwise theyll just split again next iteration resulting in a
                constant back-and-fourth and no actual refinement
            */
            const Node parentNode{leafNode.heapIndex / 2, leafNode.depth - 1};
            const uint32_t diamondParentIndex = calculateSameDepthNeighbourhood(parentNode).edge;
            const Node diamondParentNode{
                diamondParentIndex == 0u ? parentNode.heapIndex : diamondParentIndex, parentNode.depth};

            const std::array<glm::vec2, 3> parentCorners = cornersFromNode(parentNode);
            const std::array<glm::vec2, 3> diamondParentCorners = cornersFromNode(diamondParentNode);

            if(!pointInTriangle(p, parentCorners) && !pointInTriangle(p, diamondParentCorners))
            {
                mergeNodeConforming(leafNode);
            }
        }
    }
    doSumReduction();
    splitPass = !splitPass;
}

void CBT::doSumReduction()
{
    // looping until >=0 doesnt work here since level is a uint, so its always >=0
    for(uint32_t level = maxDepth - 1; level < maxDepth; level--)
    {
        const uint32_t loopStart = pow(2, level);
        const uint32_t loopEnd = pow(2, level + 1);
        for(uint32_t i = loopStart; i < loopEnd; i++)
        {
            heap[i] = heap[2 * i] + heap[2 * i + 1];
        }
    }
}

void CBT::updateDrawData()
{
    std::vector<glm::vec2> vertexPositions(heap[1] * 3);

    // after sumreduction the 1st element contains the total number of leaf nodes
    for(uint32_t i = 0; i < heap[1]; i++)
    {
        const Node leafNode = leafIndexToNode(i);

        // could build the corner data at the same time that node is calculated from the leaf index
        std::array<glm::vec2, 3> corners = cornersFromNode(leafNode);

#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
        if((leafNode.depth & 1u) != 0u)
        {
            corners = {corners[0], corners[2], corners[1]};
        }
#endif
        vertexPositions[i * 3 + 0] = corners[0];
        vertexPositions[i * 3 + 1] = corners[1];
        vertexPositions[i * 3 + 2] = corners[2];
    }

    glNamedBufferSubData(vboHandle, 0, heap[1] * 3 * sizeof(glm::vec2), vertexPositions.data());
}

void CBT::draw(const glm::mat4& projViewMatrix)
{
    shader.useProgram();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
    glBindVertexArray(vaoHandle);

    glUniform1i(1, 0);
    glDrawArrays(GL_TRIANGLES, 0, heap[1] * 3);

    // render triangle outlines with hidden line removal
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform1i(1, 1);
    glPolygonOffset(-1.0, 1.0);
    glDrawArrays(GL_TRIANGLES, 0, heap[1] * 3);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

CBT::Node CBT::nodeFromPoint(glm::vec2 p)
{
    std::array<glm::vec2, 3> corners{p0, p1, p2};
    if(!pointInTriangle(p, corners))
    {
        return {0, 0};
    }
    // point is inside 0th-level triangle, so its definitly inside one of the leaf triangles
    uint32_t currentHeapIndex = 1;
    uint32_t nodeDepth = 0;
    while(heap[currentHeapIndex] > 1)
    {
        const Corners leftChildCorners = cornersOfLeftChild(leftChildCorners);
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
    }
    assert(nodeDepth == glm::findMSB(currentHeapIndex));
    return {currentHeapIndex, nodeDepth};
}