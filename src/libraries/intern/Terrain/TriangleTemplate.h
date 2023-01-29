#pragma once

#include <intern/Mesh/Mesh.h>

#include <array>

class TriangleTemplate : public Mesh
{
  public:
    explicit TriangleTemplate(uint8_t subDivLevel);

    // not using mesh.h init since only position needs to be passed to GPU
    void init(std::span<const glm::vec2> vertices, std::span<const GLuint> indices);

    [[nodiscard]] GLuint getVAO() const;
    [[nodiscard]] uint32_t getIndexCount() const;
};