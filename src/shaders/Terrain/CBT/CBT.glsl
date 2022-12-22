#ifdef GLSLANGVALIDATOR
    #ifndef HEAP_ALREADY_DEFINED
        layout(std430, binding = 0) buffer cbtSSBO
        {   
            uint heap[];
        };
    #endif
#endif

struct Node
{
    /*
        "Virtual" heap index
        not the same as index of bits in "heap"
        representing this node
    */
    uint heapIndex;
    /*
        depth = findMSB(heapIndex)
        passed around just to save those findMSB calls
    */
    uint depth;
};
/*
    Contains the "virtual" heap indices of the neighbouring
    triangles relative to triangle "self"
*/
struct SameDepthNeighbourhood
{
    uint left;
    uint right;
    uint edge;
    uint self;
};
/*
    The bits belonging to a node in "heap" can end up across uint borders. So it may be necessary to
    read from two neighbouring array elements. This struct stores how that access happens
*/
struct ArrayAccessArguments
{
    uint firstIndex;
    uint bitOffsetInFirstIndex;
    uint bitCountInFirstIndex;
    uint secondIndex;
    uint bitCountInSecondIndex;
};