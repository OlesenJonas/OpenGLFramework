#pragma once

#include <glad/glad/glad.h>

#include <glm/ext.hpp>

#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

constexpr static GLuint VERTEX_SHADER_BIT = 1U << 0U;
constexpr static GLuint TESS_CONTROL_BIT = 1U << 1U;
constexpr static GLuint TESS_EVAL_BIT = 1U << 2U;
constexpr static GLuint GEOMETRY_SHADER_BIT = 1U << 3U;
constexpr static GLuint FRAGMENT_SHADER_BIT = 1U << 4U;
constexpr static GLuint COMPUTE_SHADER_BIT = 1U << 5U;

class ShaderProgram
{
  public:
    using DefinePair = std::pair<std::string_view, std::string_view>;

    ShaderProgram(
        GLuint shaderMask, const std::initializer_list<std::string> shaderFiles,
        const std::initializer_list<DefinePair> defines = {});
    ~ShaderProgram();

    ShaderProgram(ShaderProgram&&) = delete;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(ShaderProgram&& other) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    GLuint getProgramID();

    void useProgram();

  private:
    /* Loads a file from the give path while adding all defines in the define list
     * Stores it in the given OpenGL handle
     */
    static std::string
    loadShaderSource(std::string_view path, const std::initializer_list<DefinePair> defines = {});
    static std::string loadShaderSourceForInclude(
        std::string_view includeSourceLine, const std::filesystem::path& baseDirectory,
        std::unordered_set<std::filesystem::path>& includedSet);

    static std::string getLineDirective(uint16_t lineNumber, const std::string& path);

    bool checkShader(GLuint shaderID);
    bool checkProgram(GLuint programID);

    static const char* vendor;

    std::string shaderName;
    GLuint programID = 0xFFFFFFFF;
};
