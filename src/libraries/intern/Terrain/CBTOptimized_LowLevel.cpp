#include "CBTOptimized.h"

uint32_t CBTOptimized::getMaxDepth()
{
    return glm::findLSB(heap[0]);
}

uint32_t CBTOptimized::getNodeBitIndex(Node node)
{
    const uint32_t sum1 = 1u << (node.depth + 1);
    const uint32_t sum2 = node.heapIndex * (getMaxDepth() - node.depth + 1);
    return sum1 + sum2;
}

CBTOptimized::Node CBTOptimized::getNodeAtBitfieldDepth(Node node)
{
    const int depthOffsetToBitfieldPart = getMaxDepth() - node.depth;

    return {node.heapIndex << depthOffsetToBitfieldPart, getMaxDepth()};
}

void CBTOptimized::bitheapWriteIntoSingleChunk(
    uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount, uint32_t bitData)
{
    assert(bitOffset + bitAmount <= 32);
    const uint32_t mask = (~(0xFFFFFFFFu << bitAmount)) << bitOffset;
    heap[arrayIndex] &= ~mask;
    heap[arrayIndex] |= bitData << bitOffset;
}

uint32_t CBTOptimized::bitheapReadFromSingleChunk(uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount)
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

void CBTOptimized::bitheapWriteBits(uint32_t startIndex, uint32_t amount, uint32_t value)
{
    assert(amount <= 32);
    const ArrayAccessArguments args = calculateArrayAccesArguments(startIndex, amount);
    // because of undefined behaviour, writing 32 bits that fit perfectly into the first uint32 doesnt work.
    // not sure if also the case in glsl
    assert(amount != 32 || args.bitCountInSecondIndex != 0);
    bitheapWriteIntoSingleChunk(
        args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex, value);
    bitheapWriteIntoSingleChunk(
        args.secondIndex, 0u, args.bitCountInSecondIndex, value >> args.bitCountInFirstIndex);
}

void CBTOptimized::setNodeValue(Node node, uint32_t value)
{
    bitheapWriteBits(getNodeBitIndex(node), getMaxDepth() - node.depth + 1, value);
}

uint32_t CBTOptimized::getNodeValue(Node node)
{
    const ArrayAccessArguments args =
        calculateArrayAccesArguments(getNodeBitIndex(node), getMaxDepth() - node.depth + 1);

    const uint32_t chunk1Data =
        bitheapReadFromSingleChunk(args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex);
    const uint32_t chunk2Data = bitheapReadFromSingleChunk(args.secondIndex, 0u, args.bitCountInSecondIndex);
    const uint32_t result = chunk1Data | (chunk2Data << args.bitCountInFirstIndex);

    return result;
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