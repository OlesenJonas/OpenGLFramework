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
    /*
        Represents a node in the CBT.
        Consists of its index in the heap and its depth
    */
    struct Node
    {
        uint32_t heapIndex;
        /*
            depth = findMSB(heapIndex)
            passed around just to save those findMSB calls
        */
        uint32_t depth;
    };

    /*
        The heap indices of neighbouring triangles on the same
        subdivision depth. (Those are usually not the same
        as the actual neighbouring triangles since depths
        can vary by +-1 level)
    */
    struct SameDepthNeighbourhood
    {
        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t edge = 0;
        uint32_t self = 1;
    };

    explicit CBT(uint32_t maxDepth);

    /*
        Check if a node is a leaf node
        Disclaimer: Check comment in implementation
    */
    bool isLeafNode(Node node);
    /*
        Check if a node is the parent of two leaf nodes
    */
    bool isParentOfTwoLeafNodes(Node node);
    /*
        Split a node in a two (unsafe)
    */
    void splitNode(Node node);
    /*
        Split a node in two
        (safe, produces a conforming LEB)
    */
    void splitNodeConforming(Node node);
    /*
        Merges a node and its sibling (unsafe)
    */
    void mergeNode(Node node);
    /*
        Tries to merge a node if possible
        (safe, produces a conforming LEB)
    */
    void mergeNodeConforming(Node node);
    /*
        Calculate the neighbouring triangles of triangle B if
        B is the result of splitting triangle A.
        Takes the neighbourhood of triangle A and the direction
        of the split as parameter
    */
    SameDepthNeighbourhood neighbourhoodAfterSplit(SameDepthNeighbourhood neighbourhood, uint32_t direction);
    /*
        Calculate the neighbourhood of a Node at any depth
    */
    SameDepthNeighbourhood calculateSameDepthNeighbourhood(Node node);
    /*
        Map an index in range [0, total sum] to a Node
    */
    Node leafIndexToNode(uint32_t leafIndex);
    /*
        Calculate the corners of the triangle a node represents
    */
    std::array<glm::vec2, 3> cornersFromNode(Node node);

    /*
        Example update function: only goal is to refine the sub-
        division around the given target
    */
    void refineAroundPoint(glm::vec2 p);

    /*
        Execute the sum reduction step
    */
    void doSumReduction();

    /*
        Update the necessary draw data
        to reflect changes in the CBT
    */
    void updateDrawData();
    /*
        Executes necessary OpenGL calls to render
        the subdivision as triangles with lines
    */
    void draw(const glm::mat4& projViewMatrix);

    /*
        Helper function to retrieve leaf node containing
        the given point
    */
    Node nodeFromPoint(glm::vec2 p);

    uint32_t maxDepth = 0;
    std::vector<uint32_t> heap;

  private:
    /*
        alternatively could use a matrix3x2 type
        would also allow for calculating the corners by chaining
        matrix multiplications
    */
    using Corners = std::array<glm::vec2, 3>;
    /*
        Retrieve the value of a single bit at position bitIndex from a 32bit value
    */
    uint32_t getSingleBitValue(uint32_t field, int bitIndex);
    /*
        Calculate the corners of the left child based on the parent triangles
        corners
    */
    [[nodiscard]] Corners cornersOfLeftChild(const Corners& corners);
    /*
        Calculate the corners of the right child based on the parent triangles
        corners
    */
    [[nodiscard]] Corners cornersOfRightChild(const Corners& corners);
    /*
        Checks if a point is inside a triangle defined by the given corners
    */
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