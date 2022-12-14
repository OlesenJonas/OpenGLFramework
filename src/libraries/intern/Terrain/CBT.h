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

/*
    CBT heap is very inefficent since bits arent packed
    just for testing stuff
*/

#define CBT_VERTEX_ORDERING_PAPER 0
#define CBT_VERTEX_ORDERING_MINE 1
#define CBT_VERTEX_ORDERING CBT_VERTEX_ORDERING_PAPER

class CBT
{
  public:
    struct Node
    {
        uint32_t heapIndex;
        // depth = findMSB(heapIndex)
        // pass around just to save those findMSB calls
        uint32_t depth;
    };

    struct SameDepthNeighbourhood
    {
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t edge = 0;
        uint32_t self = 1;
    };

    explicit CBT(uint32_t maxDepth);

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
    void updateDrawData();

    void draw(const glm::mat4& projViewMatrix);
    Node nodeFromPoint(glm::vec2 p);

    uint32_t maxDepth = 0;
    std::vector<uint32_t> heap;

  private:
    uint32_t getSingleBitValue(uint32_t field, int bitID);
    void calcCornersOfLeftChild(std::array<glm::vec2, 3>& corners);
    void calcCornersOfRightChild(std::array<glm::vec2, 3>& corners);
    bool pointInTriangle(glm::vec2 pos, const std::array<glm::vec2, 3>& corners);

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

    GLuint vaoHandle = 0xffffffff;
    GLuint vboHandle = 0xffffffff;
    ShaderProgram shader;
};