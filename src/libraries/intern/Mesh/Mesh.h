#pragma once

#include <glad/glad/glad.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <span>
#include <string>
#include <vector>

struct VertexStruct
{
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 nrm = glm::vec3(0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec4 tang = glm::vec4(0.0f);
};

class Mesh
{
  public:
    Mesh() = default;
    ~Mesh();
    explicit Mesh(const std::string& path);

    Mesh(Mesh&&) = delete;
    Mesh(const Mesh&&) = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(Mesh&& other) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

  protected:
    void init(std::span<const VertexStruct> vertices, std::span<const GLuint> indices = {});

  protected:
    bool initialized = false;
    GLuint vaoHandle = 0xffffffff;
    GLuint vboHandles[2] = {0xffffffff, 0xffffffff}; // NOLINT
    GLenum indexType = GL_UNSIGNED_INT;
    uint32_t indexCount = 0;
};