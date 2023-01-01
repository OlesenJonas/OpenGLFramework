#include "CBTOptimized.h"

#include <array>

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
    doSumReductionOptimized();
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

void CBTOptimized::doSumReductionOptimized()
{
    const uint32_t maxDepth = getMaxDepth();

    // process the deepest 5 layers together (combines a whole uint32_t worth of bits into 6 bits)
    {
        const uint32_t depth = maxDepth - 5;
        const uint32_t nodesAtDepth = 1u << depth;
        const uint32_t startIndex = 1u << depth;

        for(uint32_t i = 0; i < nodesAtDepth; i++)
        {
            const uint32_t finalNodeIndex = startIndex + i;

            const uint32_t bitStart1 = getNodeBitIndex({finalNodeIndex * 32u, maxDepth});
            assert(bitStart1 % 32 == 0);
            uint32_t bitfield = heap[bitStart1 / 32u];
            uint32_t compactedData = 0u;

            // combine 32 single bits into 16 two bit numbers
            bitfield = (bitfield & 0x55555555u) + ((bitfield >> 1u) & 0x55555555u);
            compactedData = bitfield;
            const uint32_t bitStart2 = getNodeBitIndex({finalNodeIndex * 16u, maxDepth - 1u});
            assert(bitStart2 % 32 == 0);
            heap[bitStart2 / 32] = compactedData;
            // bitheapWriteBits contains a bug when data endpoints align perfectly with uint32
            // borders
            // bitheapWriteBits(bitStart2, 32, compactedData);

            // combine 16 two bit numbers into 8 three bit numbers
            bitfield = (bitfield & 0x33333333u) + ((bitfield >> 2u) & 0x33333333u);
            compactedData = ((bitfield & 0x7u) >> 0u) |       //
                            ((bitfield & 0x70u) >> 1u) |      //
                            ((bitfield & 0x700u) >> 2u) |     //
                            ((bitfield & 0x7000u) >> 3u) |    //
                            ((bitfield & 0x70000u) >> 4u) |   //
                            ((bitfield & 0x700000u) >> 5u) |  //
                            ((bitfield & 0x7000000u) >> 6u) | //
                            ((bitfield & 0x70000000u) >> 7u);
            const uint32_t bitStart3 = getNodeBitIndex({finalNodeIndex * 8u, maxDepth - 2u});
            bitheapWriteBits(bitStart3, 24, compactedData);

            // combine 8 three bit numbers into 4 four bit numbers
            bitfield = (bitfield & 0x0F0F0F0Fu) + ((bitfield >> 4u) & 0x0F0F0F0Fu);
            compactedData = ((bitfield & 0xFu) >> 0u) |     //
                            ((bitfield & 0xF00u) >> 4u) |   //
                            ((bitfield & 0xF0000u) >> 8u) | //
                            ((bitfield & 0xF000000u) >> 12u);
            const uint32_t bitStart4 = getNodeBitIndex({finalNodeIndex * 4u, maxDepth - 3u});
            bitheapWriteBits(bitStart4, 16, compactedData);

            // combine 4 four bit numbers into 2 five bit numbers
            bitfield = (bitfield & 0x000F000Fu) + ((bitfield >> 8u) & 0x000F000Fu);
            compactedData = ((bitfield & 0x1Fu) >> 0u) | //
                            ((bitfield & 0x1F0000u) >> 11u);
            const uint32_t bitStart5 = getNodeBitIndex({finalNodeIndex * 2u, maxDepth - 4u});
            bitheapWriteBits(bitStart5, 10, compactedData);

            // combine 2 five bit numbers into 1 six bit numbers
            bitfield = (bitfield & 0x0000001Fu) + ((bitfield >> 16u) & 0x0000001Fu);
            compactedData = ((bitfield & 0x3Fu) >> 0u);
            const uint32_t bitStart6 = getNodeBitIndex({finalNodeIndex * 1u, maxDepth - 5u});
            bitheapWriteBits(bitStart6, 6, compactedData);
        }
    }

    // looping until >=0 doesnt work here since level is a uint, so its always >=0
    for(uint32_t level = maxDepth - 6; level < maxDepth; level--)
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