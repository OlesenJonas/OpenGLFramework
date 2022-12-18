#include "CBTOptimized.h"
#include "Terrain/CBTOptimized.h"

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

uint32_t CBTOptimized::getSingleBitValue(uint32_t field, int bitIndex)
{
    return ((field >> bitIndex) & 1u);
}

uint32_t CBTOptimized::getMaxDepth()
{
    return glm::findLSB(heap[0]);
}

CBTOptimized::Node CBTOptimized::getNodeAtBitfieldDepth(Node node)
{
    const int depthOffsetToBitfieldPart = getMaxDepth() - node.depth;

    return {node.heapIndex << depthOffsetToBitfieldPart, getMaxDepth()};
}

void CBTOptimized::setNodeBitInBitfield(Node node, uint32_t value)
{
    assert(value <= 1u);
    const Node nodeAtBitfieldDepth = getNodeAtBitfieldDepth(node);
    // todo: could factor out into getBitIndexFromNode()
    const uint32_t bitIndex =
        (1u << (getMaxDepth() + 1u)) +
        nodeAtBitfieldDepth.heapIndex; // * nodeAtBitfieldDepth.depth (which is == 1 here);

    const uint32_t arrIndex = bitIndex / 32u;
    const uint32_t localBitIndex = bitIndex % 32u;

    const uint32_t mask = ~(1u << localBitIndex);
    heap[arrIndex] &= mask;
    heap[arrIndex] |= (value << localBitIndex);
}

uint32_t CBTOptimized::getNodeBitInBitfield(Node node)
{
    uint32_t result = 0xFFFFFFFF;

    const Node nodeAtBitfieldDepth = getNodeAtBitfieldDepth(node);
    // todo: could factor out into getBitIndexFromNode()
    const uint32_t bitIndex =
        (1u << (getMaxDepth() + 1u)) +
        nodeAtBitfieldDepth.heapIndex; // * nodeAtBitfieldDepth.depth (which is == 1 here);

    const uint32_t arrIndex = bitIndex / 32u;
    const uint32_t localBitIndex = bitIndex % 32u;

    const uint32_t mask = (1u << localBitIndex);
    result = heap[arrIndex] & mask;
    result = result >> localBitIndex;

    return result;
}

void CBTOptimized::bitheapWriteIntoChunk(
    uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount, uint32_t bitData)
{
    assert(bitOffset + bitAmount <= 32);
    const uint32_t mask = (~(0xFFFFFFFFu << bitAmount)) << bitOffset;
    heap[arrayIndex] &= ~mask;
    heap[arrayIndex] |= bitData << bitOffset;
}
uint32_t CBTOptimized::bitheapReadFromChunk(uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount)
{
    assert(bitOffset + bitAmount <= 32);
    const uint32_t mask = ~(0xFFFFFFFFu << bitAmount);
    const uint32_t result = (heap[arrayIndex] >> bitOffset) & mask;

    return result;
}

CBTOptimized::ArrayAccessArguments
CBTOptimized::calculateArrayAccesArguments(uint32_t bitIndex, uint32_t bitAmount)
{
    assert(bitAmount <= 32);
    ArrayAccessArguments args{};
    args.firstIndex = bitIndex / 32u;
    args.bitOffsetInFirstIndex = bitIndex % 32u;
    args.secondIndex = (args.bitOffsetInFirstIndex + bitAmount > 32u) ? args.firstIndex + 1 : args.firstIndex;

    args.bitCountInFirstIndex = std::min(32u - args.bitOffsetInFirstIndex, bitAmount);
    args.bitCountInSecondIndex = bitAmount - args.bitCountInFirstIndex;

    return args;
}

uint32_t CBTOptimized::getNodeBitIndex(Node node)
{
    const uint32_t sum1 = 1u << (node.depth + 1);
    const uint32_t sum2 = node.heapIndex * (getMaxDepth() - node.depth + 1);
    return sum1 + sum2;
}

void CBTOptimized::setNodeValue(Node node, uint32_t value)
{
    const ArrayAccessArguments args =
        calculateArrayAccesArguments(getNodeBitIndex(node), getMaxDepth() - node.depth + 1);

    bitheapWriteIntoChunk(args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex, value);
    bitheapWriteIntoChunk(
        args.secondIndex, 0u, args.bitCountInSecondIndex, value >> args.bitCountInFirstIndex);
}

uint32_t CBTOptimized::getNodeValue(Node node)
{
    const ArrayAccessArguments args =
        calculateArrayAccesArguments(getNodeBitIndex(node), getMaxDepth() - node.depth + 1);

    const uint32_t chunk1Data =
        bitheapReadFromChunk(args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex);
    const uint32_t chunk2Data = bitheapReadFromChunk(args.secondIndex, 0u, args.bitCountInSecondIndex);
    const uint32_t result = chunk1Data | (chunk2Data << args.bitCountInFirstIndex);

    return result;
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

/*
    this test is not 100% correct since a node and its left child
    both have the same index in the bitfield.
    That means that a "1" in the bitfield only indicates that either
    the node itself OR any of its left children are a leaf node!
*/
bool CBTOptimized::isLeafNode(Node node)
{
    return getNodeBitInBitfield(node) == 1;
}

bool CBTOptimized::isParentOfTwoLeafNodes(Node node)
{
    // A node is the parent of two leaf nodes if the sum reduction of its
    // two children is equal to 2
    return getNodeValue(node) <= 2;
}

void CBTOptimized::splitNode(Node node)
{
    setNodeBitInBitfield({2 * node.heapIndex + 1u, node.depth + 1}, 1u);
}

void CBTOptimized::splitNodeConforming(Node node)
{
    if(node.depth < getMaxDepth())
    {
        splitNode(node);
        const uint32_t edgeNeighbourID = calculateSameDepthNeighbourhood(node).edge;
        Node currentNode{edgeNeighbourID, edgeNeighbourID == 0 ? 0 : node.depth};

        // until we reach the root node, or theres no edge neighbour left
        while(currentNode.heapIndex > 1)
        {
            // split current node
            splitNode(currentNode);
            // factor out into getParent() ?
            currentNode.heapIndex = currentNode.heapIndex / 2;
            currentNode.depth -= 1;
            // split its parent
            splitNode(currentNode);
            const uint32_t edgeNeighbourID = calculateSameDepthNeighbourhood(currentNode).edge;
            // continue with parents edge neighbour
            currentNode = {edgeNeighbourID, edgeNeighbourID == 0 ? 0 : currentNode.depth};
        }
    }
}

void CBTOptimized::mergeNode(Node node)
{
    // merge
    if(node.depth > 0)
    {
        setNodeBitInBitfield({node.heapIndex | 1u, node.depth}, 0u);
    }
}

void CBTOptimized::mergeNodeConforming(Node node)
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

CBTOptimized::SameDepthNeighbourhood
CBTOptimized::neighbourhoodAfterSplit(SameDepthNeighbourhood neighbourhood, uint32_t direction) // NOLINT
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

CBTOptimized::SameDepthNeighbourhood CBTOptimized::calculateSameDepthNeighbourhood(Node node)
{
    SameDepthNeighbourhood neighbourhood{0u, 0u, 0u, 1u};

    // retrace splitting operations that result in the given node using its bit representation
    // and apply the same splits to the "base neighbourhood"
    for(int bitID = node.depth - 1; bitID >= 0; bitID--)
    {
        neighbourhood = neighbourhoodAfterSplit(neighbourhood, getSingleBitValue(node.heapIndex, bitID));
    }
    assert(node.heapIndex == 0 || node.heapIndex == neighbourhood.self);

    return neighbourhood;
}

CBTOptimized::Node CBTOptimized::leafIndexToNode(uint32_t leafIndex)
{
    // use sum reduction results to step back through tree
    // detailed in paper
    uint32_t currentHeapIndex = 1;
    uint32_t nodeDepth = 0;
    while(getNodeValue({currentHeapIndex, nodeDepth}) > 1)
    {
        const uint32_t leftChildHeapIndex = currentHeapIndex * 2;
        const uint32_t rightChildHeapIndex = currentHeapIndex * 2 + 1;
        if(leafIndex < getNodeValue({leftChildHeapIndex, nodeDepth + 1}))
        {
            currentHeapIndex = leftChildHeapIndex;
        }
        else
        {
            leafIndex -= getNodeValue({leftChildHeapIndex, nodeDepth + 1});
            currentHeapIndex = rightChildHeapIndex;
        }
        nodeDepth++;
        assert(nodeDepth == glm::findMSB(currentHeapIndex));
    }

    return {currentHeapIndex, nodeDepth};
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

void CBTOptimized::refineAroundPoint(glm::vec2 p)
{
    static bool splitPass = true;

    // after sumreduction the 1st element contains the total number of leaf nodes
    const uint32_t leafNodeAmnt = getNodeValue({1, 0});
    for(uint32_t i = 0; i < leafNodeAmnt; i++)
    {
        const Node leafNode = leafIndexToNode(i);
        /*
            writing to the heap can be done atomically
            but the CBT still cant handle cases where the same
            node is split and merged (possibly by a different node) in
            the same iteration, so alternate between both
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

void CBTOptimized::doSumReduction()
{
    const uint32_t maxDepth = getMaxDepth();
    // looping until >=0 doesnt work here since level is a uint, so its always >=0
    for(uint32_t level = maxDepth - 1; level < maxDepth; level--)
    {
        const uint32_t loopStart = pow(2, level);
        const uint32_t loopEnd = pow(2, level + 1);
        for(uint32_t i = loopStart; i < loopEnd; i++)
        {
            // heap[i] = heap[2 * i] + heap[2 * i + 1];
            setNodeValue({i, level}, getNodeValue({2 * i, level + 1}) + getNodeValue({2 * i + 1, level + 1}));
        }
    }
}

uint32_t CBTOptimized::getAmountOfLeafNodes()
{
    return getNodeValue({1, 0});
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