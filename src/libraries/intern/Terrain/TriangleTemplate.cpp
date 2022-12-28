#include "TriangleTemplate.h"

#include <array>
#include <cmath>

TriangleTemplate::TriangleTemplate(uint8_t subDivLevel)
{
    // todo: just copy pasted from cube for now. Actually create triangle geo!
    std::vector<glm::vec2> vertices;
    vertices.reserve(6);
    vertices.emplace_back(0.f, 0.f);
    vertices.emplace_back(0.5f, 0.f);
    vertices.emplace_back(1.f, 0.f);

    vertices.emplace_back(0.f, 0.5f);
    vertices.emplace_back(0.5f, 0.5f);

    vertices.emplace_back(0.f, 1.f);

    // index structure
    std::vector<GLuint> indices;
    indices.reserve(12);
    indices.emplace_back(0);
    indices.emplace_back(4);
    indices.emplace_back(3);

    indices.emplace_back(0);
    indices.emplace_back(1);
    indices.emplace_back(4);

    indices.emplace_back(1);
    indices.emplace_back(2);
    indices.emplace_back(4);

    indices.emplace_back(3);
    indices.emplace_back(4);
    indices.emplace_back(5);

    init(vertices, indices);
}

void TriangleTemplate::init(std::span<const glm::vec2> vertices, std::span<const GLuint> indices)
{
    indexCount = indices.size();

    glCreateVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glGenBuffers(2, &vboHandles[0]);

    glBindBuffer(GL_ARRAY_BUFFER, vboHandles[0]);
    glBufferStorage(GL_ARRAY_BUFFER, 2 * sizeof(float) * vertices.size(), vertices.data(), 0);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // use the second buffer as the source for indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboHandles[1]);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexCount, indices.data(), 0);

    // unbind the VBO, we don't need it anymore
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized = true;
}

GLuint TriangleTemplate::getVAO() const
{
    return vaoHandle;
}