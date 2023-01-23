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
        GLint wrapS = GL_REPEAT;
        GLint wrapT = GL_REPEAT;
    };

    Framebuffer(
        GLsizei width, GLsizei height,
        std::initializer_list<ColorAttachmentDescriptor> colorAttachmentDescriptors, bool useDepthStencil);
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