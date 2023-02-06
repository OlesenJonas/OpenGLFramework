#include "Texture.h"
#include "IO/png.h"
#include "ImageData.h"
#include "Texture/IO/png.h"
#include "Texture/Texture.h"

#include <glad/glad/glad.h>

#include <cassert>

#include <memory>
#include <stb/stb_image.h>

Texture::Texture(const std::string& file, bool imageIsInSRGB, bool mipMap)
{
    // todo: load image data first, then use descriptor based constructor

    ImageData imageData;
    auto extensionStart = file.find_last_of('.');
    const std::string extension = file.substr(extensionStart, file.size());

    if(extension == ".png")
    {
        imageData = png::read(file, imageIsInSRGB, true);
        assert(imageData.data.get() != nullptr);
    }
    else
    {
        //  load texture data from file
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(true);

        bool isHdr = stbi_is_hdr(file.c_str()) != 0;

        unsigned char* image = nullptr;
        if(!isHdr)
        {
            image = stbi_load(file.c_str(), &width, &height, &channels, 0);
            // default to treating nonHDR textures as sRGB for now
            imageData.pixelType = imageIsInSRGB ? Texture::PixelType::UCHAR_SRGB : Texture::PixelType::UCHAR;
        }
        else
        {
            stbi_ldr_to_hdr_gamma(1.0f);
            image = (unsigned char*)stbi_loadf(file.c_str(), &width, &height, &channels, 0);
            imageData.pixelType = Texture::PixelType::FLOAT;
        }

        imageData.width = width;
        imageData.height = height;
        switch(channels)
        {
        case 1:
            imageData.channels = Texture::Channels::R;
            break;
        case 2:
            imageData.channels = Texture::Channels::RG;
            break;
        case 3:
            imageData.channels = Texture::Channels::RGB;
            break;
        case 4:
            imageData.channels = Texture::Channels::RGBA;
            break;
        default:
            assert(false);
        }

        if(channels == 0 || width == 0 || height == 0)
        {
            std::cout << "Could not load texture " << file << std::endl;
            return;
        }
        const int bpp = isHdr ? 4 : 1;

        // kind of ugly but so are the stbi interfaces :shrug:
        imageData.data = std::make_unique<unsigned char[]>(width * height * channels * bpp);
        memcpy(imageData.data.get(), image, width * height * channels * bpp);
        stbi_image_free(image);
    }

    const auto separatorPos = file.find_last_of('/') + 1;
    const std::string texName = file.substr(separatorPos, file.find_last_of('.') - separatorPos);

    this->width = imageData.width;
    this->height = imageData.height;

    const int levels = static_cast<int>(log2(std::max(width, height))) + 1;

    // create openGL texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
    glObjectLabel(GL_TEXTURE, textureID, static_cast<GLsizei>(texName.size()), texName.data());

    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if(mipMap)
    {
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GLfloat maxAniso = 0;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTextureParameterf(textureID, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    }
    else
    {
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTextureStorage2D(
        textureID, levels, getGLFormat(imageData.channels, imageData.pixelType), width, height);
    glTextureSubImage2D(
        textureID,
        0,
        0,
        0,
        width,
        height,
        getGLChannel(imageData.channels),
        getGLPixelType(imageData.pixelType),
        imageData.data.get());

    const auto test = GL_FLOAT;

    if(mipMap)
    {
        glGenerateTextureMipmap(textureID);
    }

    initialized = true;
}

Texture::Texture(const TextureDesc descriptor) : width(descriptor.width), height(descriptor.height)
{

    glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
    glTextureStorage2D(
        textureID, descriptor.levels, descriptor.internalFormat, descriptor.width, descriptor.height);
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, descriptor.minFilter);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, descriptor.magFilter);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, descriptor.wrapS);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, descriptor.wrapT);
	glTextureParameteri(textureID, GL_TEXTURE_COMPARE_FUNC, descriptor.compFunc);
	glTextureParameteri(textureID, GL_TEXTURE_COMPARE_MODE, descriptor.compMode);

    // GLfloat maxAniso = 0;
    // glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    // glTextureParameterf(handle, GL_TEXTURE_MAX_ANISOTROPY, std::max(maxAniso, descriptor.maxAnisotropy));
    if(strlen(descriptor.name) > 0)
    {
        glObjectLabel(GL_TEXTURE, textureID, -1, descriptor.name);
    }
    if(descriptor.data != nullptr)
    {
        glTextureSubImage2D(
            textureID, 0, 0, 0, width, height, descriptor.dataFormat, descriptor.dataType, descriptor.data);
    }
    if(descriptor.generateMips)
    {
        glGenerateTextureMipmap(textureID);
    }
    initialized = true;
}

Texture::Texture(Texture&& other) noexcept : width(other.width), height(other.height)
{
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    width = other.width;
    height = other.height;
    // if this texture was already initialized then somebody needs to delete the old content
    // so swap IDs and init staus instead of just moving and *other will delete on destruction
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
    return *this;
}

Texture::~Texture()
{
    if(initialized)
    {
        glDeleteTextures(1, &textureID);
    }
}

void Texture::swap(Texture& other)
{
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(textureID, other.textureID);
    std::swap(initialized, other.initialized);
}