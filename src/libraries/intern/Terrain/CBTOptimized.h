#pragma once

#include <cstdint>
#include <vector>

#include <intern/ShaderProgram/ShaderProgram.h>

#include <glad/glad/glad.h>
#include <glm/glm.hpp>

/*
  based on: https://onrendering.com/data/papers/cbt/ConcurrentBinaryTrees.pdf
  Concurrent Binary Trees (with application to longest edge bisection)
    by Jonathan Dupuy
*/

// comments in CBT.h

#define CBT_VERTEX_ORDERING_PAPER 0
#define CBT_VERTEX_ORDERING_MINE 1
#define CBT_VERTEX_ORDERING CBT_VERTEX_ORDERING_PAPER

class CBTOptimized
{
  public:
    struct Node
    {
        uint32_t heapIndex;
        /*
            depth = findMSB(heapIndex)
            passed around just to save those findMSB calls
        */
        uint32_t depth;
    };

    struct SameDepthNeighbourhood
    {
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t edge = 0;
        uint32_t self = 1;
    };

    explicit CBTOptimized(uint32_t maxDepth);

    bool isLeafNode(Node node);
    bool isParentOfTwoLeafNodes(Node node);
    void splitNode(Node node);
    void splitNodeConforming(Node node);
    void mergeNode(Node node);
    void mergeNodeConforming(Node node);

    SameDepthNeighbourhood neighbourhoodAfterSplit(SameDepthNeighbourhood neighbourhood, uint32_t direction);
    SameDepthNeighbourhood calculateSameDepthNeighbourhood(Node node);

    Node leafIndexToNode(uint32_t leafIndex);

    std::array<glm::vec2, 3> cornersFromNode(Node node);

    void refineAroundPoint(glm::vec2 p);

    void doSumReduction();

    uint32_t getAmountOfLeafNodes();

    void updateDrawData();
    void draw(const glm::mat4& projViewMatrix);

    Node nodeFromPoint(glm::vec2 p);

    uint32_t maxDepth = 0;

    Node getNodeAtBitfieldDepth(Node node);

    void setNodeBitInBitfield(Node node, uint32_t value);
    uint32_t getNodeBitInBitfield(Node node);

    void bitheapWriteIntoChunk(uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount, uint32_t bitData);
    uint32_t bitheapReadFromChunk(uint32_t arrayIndex, uint32_t bitOffset, uint32_t bitAmount);
    // the bits belonging to a node can end up across uint32_t borders. So it may
    // be necessary to read from two neighbouring array elements. This struct
    // stores how that access happens
    struct ArrayAccessArguments
    {
        uint32_t firstIndex;
        uint32_t bitOffsetInFirstIndex;
        uint32_t bitCountInFirstIndex;
        uint32_t secondIndex;
        uint32_t bitCountInSecondIndex;
    };
    ArrayAccessArguments calculateArrayAccesArguments(uint32_t bitIndex, uint32_t bitAmount);
    uint32_t getNodeBitIndex(Node node);
    void setNodeValue(Node node, uint32_t value);
    uint32_t getNodeValue(Node node);

    uint32_t getSingleBitValue(uint32_t field, int bitIndex);

  private:
    std::vector<uint32_t> heap;

    using Corners = std::array<glm::vec2, 3>;
    [[nodiscard]] Corners cornersOfLeftChild(const Corners& corners);
    [[nodiscard]] Corners cornersOfRightChild(const Corners& corners);
    bool pointInTriangle(glm::vec2 pos, const Corners& corners);

#if CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_MINE
    // corner points, counter clockwise, starting with 1st point on base
    // in world space xz plane
    glm::vec2 p0{1.0f, 1.0f};
    glm::vec2 p1{-1.0f, -1.0f};
    glm::vec2 p2{-1.0f, 1.0f};
#elif CBT_VERTEX_ORDERING == CBT_VERTEX_ORDERING_PAPER
    glm::vec2 p0{-1.0f, -1.0f};
    glm::vec2 p1{-1.0f, 1.0f};
    glm::vec2 p2{1.0f, 1.0f};
#endif

    /* For OpenGL rendering */
    GLuint vaoHandle = 0xffffffff;
    GLuint vboHandle = 0xffffffff;
    ShaderProgram shader;
};