#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#include "CBT.glsl"

uint getSingleBitValue(const uint field,const int bitIndex)
{
    return ((field >> bitIndex) & 1u);
}

int getMaxDepth()
{
    return findLSB(heap[0]);
}

uint getNodeBitIndex(const Node node)
{
    const uint sum1 = 1u << (node.depth + 1u);
    const uint sum2 = node.heapIndex * (getMaxDepth() - node.depth + 1);
    return sum1 + sum2;
}

Node getNodeAtBitfieldDepth(const Node node)
{
    const int depthOffsetToBitfieldPart = getMaxDepth() - int(node.depth);

    return Node(node.heapIndex << depthOffsetToBitfieldPart, getMaxDepth());
}

void bitheapWriteIntoSingleChunk(
    const uint arrayIndex, const uint bitOffset, const uint bitAmount, const uint bitData)
{
    const uint mask = (~(0xFFFFFFFFu << bitAmount)) << bitOffset;
    // heap[arrayIndex] &= ~mask;
    atomicAnd(heap[arrayIndex],~mask);
    // heap[arrayIndex] |= bitData << bitOffset;
    atomicOr(heap[arrayIndex], bitData << bitOffset);
}

uint bitheapReadFromSingleChunk(const uint arrayIndex, const uint bitOffset, const uint bitAmount)
{
    const uint mask = ~(0xFFFFFFFFu << bitAmount);
    const uint result = (heap[arrayIndex] >> bitOffset) & mask;

    return result;
}

ArrayAccessArguments
calculateArrayAccesArguments(const uint bitIndex, const uint bitAmount)
{
    ArrayAccessArguments args;
    args.firstIndex = bitIndex / 32u;
    args.bitOffsetInFirstIndex = bitIndex % 32u;
    args.secondIndex = (args.bitOffsetInFirstIndex + bitAmount > 32u) ? args.firstIndex + 1 : args.firstIndex;

    args.bitCountInFirstIndex = min(32u - args.bitOffsetInFirstIndex, bitAmount);
    args.bitCountInSecondIndex = bitAmount - args.bitCountInFirstIndex;

    return args;
}

void bitheapWriteBits(const uint startIndex, const uint amount, const uint value)
{
    const ArrayAccessArguments args = calculateArrayAccesArguments(startIndex, amount);
    bitheapWriteIntoSingleChunk(
        args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex, value);
    bitheapWriteIntoSingleChunk(
        args.secondIndex, 0u, args.bitCountInSecondIndex, value >> args.bitCountInFirstIndex);
}

void setNodeValue(const Node node, const uint value)
{
    bitheapWriteBits(getNodeBitIndex(node), getMaxDepth() - node.depth + 1, value);
}

uint getNodeValue(const Node node)
{
    const ArrayAccessArguments args =
        calculateArrayAccesArguments(getNodeBitIndex(node), getMaxDepth() - node.depth + 1);

    const uint chunk1Data =
        bitheapReadFromSingleChunk(args.firstIndex, args.bitOffsetInFirstIndex, args.bitCountInFirstIndex);
    const uint chunk2Data = bitheapReadFromSingleChunk(args.secondIndex, 0u, args.bitCountInSecondIndex);
    const uint result = chunk1Data | (chunk2Data << args.bitCountInFirstIndex);

    return result;
}

void setNodeBitInBitfield(const Node node, const uint value)
{
    const Node nodeAtBitfieldDepth = getNodeAtBitfieldDepth(node);
    const uint bitIndex =
        (1u << (getMaxDepth() + 1u)) +
        nodeAtBitfieldDepth.heapIndex; // * nodeAtBitfieldDepth.depth (which is == 1 here);

    const uint arrIndex = bitIndex / 32u;
    const uint localBitIndex = bitIndex % 32u;

    const uint mask = ~(1u << localBitIndex);
    // heap[arrIndex] &= mask;
    atomicAnd(heap[arrIndex], mask);
    // heap[arrIndex] |= (value << localBitIndex);
    atomicOr(heap[arrIndex], value << localBitIndex);
}

uint getNodeBitInBitfield(const Node node)
{
    uint result = 0xFFFFFFFF;

    const Node nodeAtBitfieldDepth = getNodeAtBitfieldDepth(node);
    // todo: could factor out into getBitIndexFromNode()
    const uint bitIndex =
        (1u << (getMaxDepth() + 1u)) +
        nodeAtBitfieldDepth.heapIndex; // * nodeAtBitfieldDepth.depth (which is == 1 here);

    const uint arrIndex = bitIndex / 32u;
    const uint localBitIndex = bitIndex % 32u;

    const uint mask = (1u << localBitIndex);
    result = heap[arrIndex] & mask;
    result = result >> localBitIndex;

    return result;
}