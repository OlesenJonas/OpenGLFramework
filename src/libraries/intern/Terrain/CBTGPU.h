#pragma once

#include <cstdint>
#include <vector>

#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/TriangleTemplate.h>

#include <glad/glad/glad.h>
#include <glm/glm.hpp>

/*
  based on: https://onrendering.com/data/papers/cbt/ConcurrentBinaryTrees.pdf
  Concurrent Binary Trees (with application to longest edge bisection)
    by Jonathan Dupuy
*/

class CBTGPU
{

  public:
    explicit CBTGPU(uint32_t maxDepth);

    void update(glm::vec2 point);
    void draw(const glm::mat4& projViewMatrix);
    void drawOutline(const glm::mat4& projViewMatrix);

  private:
    uint32_t maxDepth = 0xFFFFFFFF;
    GLuint cbtBuffer = 0xFFFFFFFF;
    uint64_t cbtBufferByteSize = 0;
    GLuint indirectDispatchCommandBuffer = 0xFFFFFFFF;
    GLuint indirectDrawCommandBuffer = 0xFFFFFFFF;
    ShaderProgram cbtUpdateShader;

    TriangleTemplate triangleMesh;
    ShaderProgram drawShader;
};