#include "TriangleTemplate.h"

#include <array>
#include <cmath>

// sum from 1 to N
int sumUpTo(int N) // NOLINT
{
    return (N * (N + 1)) / 2;
}

TriangleTemplate::TriangleTemplate(uint8_t subDivLevel)
{
    if(subDivLevel == 0)
    {
        std::vector<glm::vec2> vertices = {{0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}};
        std::vector<GLuint> indices = {0, 1, 2};
        init(vertices, indices);
        return;
    }

    int d = subDivLevel * 2; // NOLINT

    std::vector<glm::vec2> vertices;
    vertices.reserve(sumUpTo(d + 1));

    for(int row = 0; row <= d; row++)
    {
        float zcoord = float(row) / float(d);
        for(int col = 0; col <= d - row; col++)
        {
            float xcoord = float(col) / float(d);
            vertices.emplace_back(xcoord, zcoord);
        }
    }

    // set the corner positions manually to make sure
    // theres no float inaccuracies and triangles line up perfectly
    // probably not really needed, just to make sure
    vertices[0] = {0.f, 0.f};
    vertices[d] = {1.f, 0.f};
    vertices[vertices.size() - 1] = {0.f, 1.f};

    // index structure
    std::vector<GLuint> indices;
    indices.reserve((1u << d) * 3);

    int bottomRowStartIndex = 0;
    for(int row = 0; row < d; row++)
    {
        int topRowStartIndex = bottomRowStartIndex + (d - row + 1);

        int fullQuads = d - row - 1;

        for(int i = 0; i < fullQuads; i++)
        {
            const bool diagonalIsRising = (i % 2) == (row % 2);
            if(diagonalIsRising)
            {
                indices.emplace_back(bottomRowStartIndex + i);
                indices.emplace_back(topRowStartIndex + i + 1);
                indices.emplace_back(topRowStartIndex + i);

                indices.emplace_back(bottomRowStartIndex + i);
                indices.emplace_back(bottomRowStartIndex + i + 1);
                indices.emplace_back(topRowStartIndex + i + 1);
            }
            else
            {
                indices.emplace_back(bottomRowStartIndex + i);
                indices.emplace_back(bottomRowStartIndex + i + 1);
                indices.emplace_back(topRowStartIndex + i);

                indices.emplace_back(topRowStartIndex + i);
                indices.emplace_back(bottomRowStartIndex + i + 1);
                indices.emplace_back(topRowStartIndex + i + 1);
            }
        }
        // last triangle finishing row
        indices.emplace_back(bottomRowStartIndex + fullQuads);
        indices.emplace_back(bottomRowStartIndex + fullQuads + 1);
        indices.emplace_back(topRowStartIndex + fullQuads);

        bottomRowStartIndex = topRowStartIndex;
    }

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

uint32_t TriangleTemplate::getIndexCount() const
{
    return indexCount;
}