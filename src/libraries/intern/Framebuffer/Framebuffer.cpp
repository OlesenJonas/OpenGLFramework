#include "Framebuffer.h"
#include "Texture/GLTexture.h"

#include <Misc/OpenGLErrorHandler.h>

#include <cassert>

Framebuffer::Framebuffer(
    GLsizei width, GLsizei height,
    std::initializer_list<ColorAttachmentDescriptor> colorAttachmentDescriptors, bool useDepthStencil)
    : width(width), height(height), hasDepthStencilAttachment(useDepthStencil)
{
    textures.reserve(colorAttachmentDescriptors.size() + (int)useDepthStencil);

    glCreateFramebuffers(1, &handle);

    int index = 0;
    for(const auto& descriptor : colorAttachmentDescriptors)
    {
        const Texture& newTex = textures.emplace_back(TextureDesc{
            .levels = descriptor.levels,
            .width = width,
            .height = height,
            .internalFormat = descriptor.internalFormat,
            .minFilter = descriptor.minFilter,
            .magFilter = descriptor.magFilter,
            .wrapS = descriptor.wrapS,
            .wrapT = descriptor.wrapT});
        glNamedFramebufferTexture(handle, GL_COLOR_ATTACHMENT0 + index, newTex.getTextureID(), 0);
        index++;
    }
    if(useDepthStencil)
    {
        Texture& newTex = textures.emplace_back(
            TextureDesc{.width = width, .height = height, .internalFormat = GL_DEPTH_COMPONENT32F});
        // glNamedFramebufferTexture(handle, GL_DEPTH_STENCIL_ATTACHMENT, newTex.getTextureID(), 0);
        glNamedFramebufferTexture(handle, GL_DEPTH_ATTACHMENT, newTex.getTextureID(), 0);
    }

    std::vector<GLenum> attachments(colorAttachmentDescriptors.size());
    assert(colorAttachmentDescriptors.size() == attachments.size());
    for(int i = 0; i < attachments.size(); i++)
    {
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glNamedFramebufferDrawBuffers(handle, (int)attachments.size(), attachments.data());

    if(glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printOpenGLErrors();
        assert(false && "Framebuffer incomplete!");
        // todo: also do something in non-debug builds!
    }
}

Framebuffer::Framebuffer(
    GLsizei width, GLsizei height,
    const std::initializer_list<std::pair<const GLTexture&, int>> texturesWithLevels, bool useDepthStencil)
    : width(width), height(height), hasDepthStencilAttachment(useDepthStencil)
{
    glCreateFramebuffers(1, &handle);

    int index = 0;
    for(const auto& textureAndLevel : texturesWithLevels)
    {
        const GLTexture& texture = textureAndLevel.first;
        const int mipLevel = textureAndLevel.second;
        int w, h;
        glGetTextureLevelParameteriv(texture.getTextureID(), mipLevel, GL_TEXTURE_WIDTH, &w);
        glGetTextureLevelParameteriv(texture.getTextureID(), mipLevel, GL_TEXTURE_HEIGHT, &h);
        // TODO: writing into just a portion of the texture could be desired, so just asserting here isnt
        // optimal
        assert(w == width && h == height);

        glNamedFramebufferTexture(handle, GL_COLOR_ATTACHMENT0 + index, texture.getTextureID(), mipLevel);
        index++;
    }
    if(useDepthStencil)
    {
        Texture& newTex = this->textures.emplace_back(
            TextureDesc{.width = width, .height = height, .internalFormat = GL_DEPTH_COMPONENT32F});
        // glNamedFramebufferTexture(handle, GL_DEPTH_STENCIL_ATTACHMENT, newTex.getTextureID(), 0);
        glNamedFramebufferTexture(handle, GL_DEPTH_ATTACHMENT, newTex.getTextureID(), 0);
    }

    std::vector<GLenum> attachments(texturesWithLevels.size());
    for(int i = 0; i < attachments.size(); i++)
    {
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glNamedFramebufferDrawBuffers(handle, (int)attachments.size(), attachments.data());

    if(glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printOpenGLErrors();
        assert(false && "Framebuffer incomplete!");
        // todo: also do something in non-debug builds!
    }
}

Framebuffer::Framebuffer(
    GLsizei width, GLsizei height,
    const std::initializer_list<std::pair<const GLTexture&, int>> texturesWithLevels,
    std::pair<const GLTexture&, int> depthAttachment)
    : width(width), height(height), hasDepthStencilAttachment(true)
{
    glCreateFramebuffers(1, &handle);

    int index = 0;
    for(const auto& textureAndLevel : texturesWithLevels)
    {
        const GLTexture& texture = textureAndLevel.first;
        const int mipLevel = textureAndLevel.second;
        int w, h;
        glGetTextureLevelParameteriv(texture.getTextureID(), mipLevel, GL_TEXTURE_WIDTH, &w);
        glGetTextureLevelParameteriv(texture.getTextureID(), mipLevel, GL_TEXTURE_HEIGHT, &h);
        // TODO: writing into just a portion of the texture could be desired, so just asserting here isnt
        // optimal
        assert(w == width && h == height);

        glNamedFramebufferTexture(handle, GL_COLOR_ATTACHMENT0 + index, texture.getTextureID(), mipLevel);
        index++;
    }

    glNamedFramebufferTexture(
        handle, GL_DEPTH_ATTACHMENT, depthAttachment.first.getTextureID(), depthAttachment.second);

    std::vector<GLenum> attachments(texturesWithLevels.size());
    for(int i = 0; i < attachments.size(); i++)
    {
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glNamedFramebufferDrawBuffers(handle, (int)attachments.size(), attachments.data());

    if(glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printOpenGLErrors();
        assert(false && "Framebuffer incomplete!");
        // todo: also do something in non-debug builds!
    }
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
{
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    width = other.width;
    height = other.height;
    hasDepthStencilAttachment = other.hasDepthStencilAttachment;
    std::swap(handle, other.handle);
    textures = std::move(other.textures);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    width = other.width;
    height = other.height;
    hasDepthStencilAttachment = other.hasDepthStencilAttachment;
    std::swap(handle, other.handle);
    textures = std::move(other.textures);
    return *this;
}

Framebuffer::~Framebuffer()
{
    if(handle != 0xFFFFFFFF)
    {
        glDeleteFramebuffers(1, &handle);
    }
}

void Framebuffer::bind() const
{
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

/*
    todo: return textures even if framebuffer is non-owning!
*/
const std::vector<Texture>& Framebuffer::getColorTextures() const
{
    return textures;
}

const Texture* Framebuffer::getDepthTexture() const
{
    if(!hasDepthStencilAttachment)
    {
        return nullptr;
    }
    return &textures[textures.size() - 1];
}