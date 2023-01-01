#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#include "LowLevel.glsl"

SameDepthNeighbourhood
neighbourhoodAfterSplit(const SameDepthNeighbourhood neighbourhood, const uint direction)
{
    // 0 represents "null" and null+1 needs to stay 0 / null
    const uint summandLeft = (neighbourhood.left == 0u) ? 0u : 1u;
    const uint summandRight = (neighbourhood.right == 0u) ? 0u : 1u;
    const uint summandEdge = (neighbourhood.edge == 0u) ? 0u : 1u;

    if(direction == 0u)
    {
        return SameDepthNeighbourhood(
            2 * neighbourhood.self + 1, // self is always != null
            2 * neighbourhood.edge + summandEdge,
            2 * neighbourhood.right + summandRight,
            2 * neighbourhood.self);
    }
    if(direction == 1u)
    {
        return SameDepthNeighbourhood(
            2 * neighbourhood.edge,
            2 * neighbourhood.self,
            2 * neighbourhood.left,
            2 * neighbourhood.self + 1);
    }
    
    //should not be reached
    return SameDepthNeighbourhood(0,0,0,0);
}

SameDepthNeighbourhood calculateSameDepthNeighbourhood(const Node node)
{
    SameDepthNeighbourhood neighbourhood = SameDepthNeighbourhood(0u, 0u, 0u, 1u);

    // retrace splitting operations that result in the given node using its bit representation
    // and apply the same splits to the "base neighbourhood"
    for(int bitID = int(node.depth) - 1; bitID >= 0; bitID--)
    {
        neighbourhood = neighbourhoodAfterSplit(neighbourhood, getSingleBitValue(node.heapIndex, bitID));
    }

    return neighbourhood;
}

uint getAmountOfLeafNodes()
{
    return getNodeValue(Node(1, 0));
}


Node leafIndexToNode(uint leafIndex)
{
    // use sum reduction results to step back through tree
    // detailed in paper
    Node iterator = Node(1, 0);
    while(getNodeValue(iterator) > 1)
    {
        const uint leftChildHeapIndex = iterator.heapIndex * 2;
        const uint rightChildHeapIndex = leftChildHeapIndex + 1;
        const uint leftChildValue = getNodeValue(Node(leftChildHeapIndex, iterator.depth + 1));
        if(leafIndex < leftChildValue)
        {
            iterator.heapIndex = leftChildHeapIndex;
        }
        else
        {
            leafIndex -= leftChildValue;
            iterator.heapIndex = rightChildHeapIndex;
        }
        iterator.depth++;
    }

    return iterator;
}

/*
    this test is not 100% correct since a node and its left child
    both have the same index in the bitfield.
    That means that a "1" in the bitfield only indicates that either
    the node itself OR any of its left children are a leaf node!
*/
bool isLeafNode(const Node node)
{
    return getNodeBitInBitfield(node) == 1;
}

bool isParentOfTwoLeafNodes(const Node node)
{
    // A node is the parent of two leaf nodes if the sum reduction of its
    // two children is equal to 2
    return getNodeValue(node) <= 2;
}

#ifndef HEAP_READ_ONLY

    void splitNode(const Node node)
    {
        setNodeBitInBitfield(Node(2 * node.heapIndex + 1u, node.depth + 1), 1u);
    }

    void splitNodeConforming(const Node node)
    {
        if(node.depth < getMaxDepth())
        {
            splitNode(node);
            const uint edgeNeighbourID = calculateSameDepthNeighbourhood(node).edge;
            Node currentNode = Node(edgeNeighbourID, edgeNeighbourID == 0 ? 0 : node.depth);

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
                const uint edgeNeighbourID = calculateSameDepthNeighbourhood(currentNode).edge;
                // continue with parents edge neighbour
                // currentNode = Node(edgeNeighbourID, edgeNeighbourID == 0 ? 0 : currentNode.depth);
                currentNode.heapIndex = edgeNeighbourID;
                currentNode.depth = edgeNeighbourID == 0 ? 0 : currentNode.depth;
            }
        }
    }

    void mergeNode(const Node node)
    {
        // merge
        if(node.depth > 0)
        {
            setNodeBitInBitfield(Node(node.heapIndex | 1u, node.depth), 0u);
        }
    }

    void mergeNodeConforming(const Node node)
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
        const Node parentNode = Node(node.heapIndex / 2, node.depth - 1);
        const bool canSplitTop = isParentOfTwoLeafNodes(parentNode);

        const uint diamondIndex = calculateSameDepthNeighbourhood(parentNode).edge;
        // set diamond index = parentIndex if diamondIndex doesnt exist, saves some braching later on
        const Node diamondNode = Node(diamondIndex == 0 ? parentNode.heapIndex : diamondIndex, parentNode.depth);
        const bool canSplitBot = isParentOfTwoLeafNodes(diamondNode);

        if(canSplitTop && canSplitBot)
        {
            mergeNode(node);
            const Node rightDiamondChild = Node(2 * diamondNode.heapIndex + 1, diamondNode.depth + 1);
            mergeNode(rightDiamondChild);
        }
    }

#endif