#include "CBTOptimized.h"

uint32_t CBTOptimized::getAmountOfLeafNodes()
{
    return getNodeValue({1, 0});
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