#pragma once

#include <glad/glad/glad.h>

#include <initializer_list>
#include <vector>

#include <intern/Texture/Texture.h>

class Framebuffer
{
  public:
    struct ColorAttachmentDescriptor
    {
        GLsizei levels = 1;
        GLsizei width = -1;
        GLsizei height = -1;
        GLenum internalFormat = 0xFFFFFFFF;
        GLint minFilter = GL_LINEAR;
        GLint magFilter = GL_LINEAR;
        GLint wrapS = GL_CLAMP_TO_EDGE;
        GLint wrapT = GL_CLAMP_TO_EDGE;
    };

    /*
      Construct an "empty" Framebuffer object
    */
    Framebuffer() = default;

    /*
      Construct a framebuffer that owns its rendertargets
    */
    Framebuffer(
        GLsizei width, GLsizei height,
        std::initializer_list<ColorAttachmentDescriptor> colorAttachmentDescriptors, bool useDepthStencil);
    /*
      Construct a framebuffer that uses existing textures as rendertargets
      Does not do any lifetime checks!
    */
    Framebuffer(
        GLsizei width, GLsizei height,
        const std::initializer_list<std::pair<const GLTexture&, int>> texturesWithLevels,
        bool useDepthStencil);
    Framebuffer(Framebuffer&& other) noexcept;            // Move constructor
    Framebuffer& operator=(Framebuffer&& other) noexcept; // Move assignment
    Framebuffer(const Framebuffer&) = delete;             // copy constr
    Framebuffer& operator=(const Framebuffer&) = delete;  // copy assign
    ~Framebuffer();

    void bind() const;

    [[nodiscard]] const std::vector<Texture>& getColorTextures() const;
    // todo: Unsure about optional refernces so just return raw pointer instead
    [[nodiscard]] const Texture* getDepthTexture() const;

  private:
    GLsizei width = -1;
    GLsizei height = -1;
    bool hasDepthStencilAttachment = false;
    GLuint handle = 0xFFFFFFFF;
    std::vector<Texture> textures;
};