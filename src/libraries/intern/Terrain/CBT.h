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
    struct SameDepthNeighbourhood
    {
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t edge = 0;
        uint32_t self = 1;
    };

    explicit CBT(uint32_t maxDepth);

    bool isLeafNode(uint32_t heapIndex);
    bool isParentOfTwoLeafNodes(uint32_t heapIndex);
    void splitNode(uint32_t heapIndex);
    void splitNodeConforming(uint32_t heapIndex);
    void mergeNode(uint32_t heapIndex);
    void mergeNodeConforming(uint32_t heapIndex);
    SameDepthNeighbourhood calculateSameDepthNeighbourhood(uint32_t heapIndex);
    SameDepthNeighbourhood neighbourhoodAfterSplit(SameDepthNeighbourhood neighbourhood, uint32_t direction);
    uint32_t leafIndexToHeapIndex(uint32_t leafIndex);
    void doSumReduction();
    void updateDrawData();

    void draw(const glm::mat4& projViewMatrix);
    uint32_t heapIndexFromPoint(glm::vec2 p);

    uint32_t maxDepth = 0;
    std::vector<uint32_t> heap;

  private:
    uint32_t getSingleBitValue(uint32_t field, int bitID);

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