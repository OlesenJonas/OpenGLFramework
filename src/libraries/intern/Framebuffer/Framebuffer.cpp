#include "Framebuffer.h"

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

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &handle);
}

void Framebuffer::bind() const
{
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

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