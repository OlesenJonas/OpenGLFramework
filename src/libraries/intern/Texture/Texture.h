#pragma once

#include <glad/glad/glad.h>

#include <array>
#include <iostream>
#include <string>

#include "GLTexture.h"

struct TextureDesc
{
    const char* name = "";
    GLsizei levels = 1;
    GLsizei width = -1;
    GLsizei height = -1;
    GLsizei depth = -1;
    GLenum internalFormat = 0xFFFFFFFF;
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;
    GLint wrapS = GL_REPEAT;
    GLint wrapT = GL_REPEAT;
    GLint wrapR = GL_REPEAT;
    const uint8_t* data = nullptr;
    GLenum dataFormat = 0xFFFFFFFF;
    GLenum dataType = 0xFFFFFFFF;
    bool generateMips = false;
	GLint compFunc = GL_LEQUAL;
	GLint compMode = GL_NONE;
};

class Texture : public GLTexture
{
  public:
    /** delete any copy ctor/assign since the implicitly existing ones
     * would only copy the opengl handle and not the full texture
     * meaning if one gets destructed both objects would have an invalid handle
     * and I dont want to deal with that right now
     */
    Texture(const Texture&) = delete;            // copy constr
    Texture& operator=(const Texture&) = delete; // copy assign

    /** Sets up and loads a texture from a file.
     * @param file Name of the image file within the resource folder
     * @param imageIsInSRGB Is the image data in the file in sRGB color space
     * @param mipMap True if texture is supposed to generate mipmapping
     */
    Texture(const std::string& file, bool imageIsInSRGB, bool mipMap = true);

    /** Creates an immutable Texture object based on a given descriptor.
     * @param descriptor Descriptor to use for configuring the texture
     */
    explicit Texture(TextureDesc descriptor);

    /*
        Move constructor

    */
    Texture(Texture&& other) noexcept;
    /*
        Move assignment
    */
    Texture& operator=(Texture&& other) noexcept;

    /** Deletes the OpenGL texture object.
     */
    ~Texture();

    void swap(Texture& other);

    inline int getWidth()
    {
        return width;
    };

    inline int getHeight()
    {
        return height;
    };

  private:
    /*
        does not actually store the descriptor (not needed atm)
        if that changes in the future, then make sure that move ctor, move assign and swap
        also move the descriptor
    */
    bool initialized = false;
    int width = -1;
    int height = -1;

  public:
    /*
        Utility functions to convert between generic image data and texture formats etc
        saves a lot of ifs/switches to determine to correct gl format when given just a number of channels and
        a pixel type
    */
    // todo: expand as needed
    enum Channels : uint8_t
    {
        R,
        RG,
        RGB,
        RGBA,
        C_COUNT,
    };

    enum PixelType : uint8_t
    {
        UCHAR,
        UCHAR_SRGB,
        USHORT,
        FLOAT,
        PT_COUNT,
    };

    constexpr static inline GLenum getGLFormat(Channels channels, PixelType type)
    {
        return glFormatLUT[type][channels];
    }

    constexpr static inline GLenum getGLChannel(Channels channels)
    {
        return glChannelLUT[channels];
    }

    constexpr static inline GLenum getGLPixelType(PixelType type)
    {
        return glPixelTypeLUT[type];
    }

  private:
    constexpr static std::array<uint8_t, PT_COUNT> pixelByteSizeLUT = {1, 1, 2, 4};

    constexpr static std::array<std::array<GLenum, C_COUNT>, PT_COUNT> glFormatLUT = {{
        {{GL_R8, GL_RG8, GL_RGB8, GL_RGBA8}},
        {{0xFFFFFFFF, 0xFFFFFFFF, GL_SRGB8, GL_SRGB8_ALPHA8}},
        {{GL_R16, GL_RG16, GL_RGB16, GL_RGBA16}},
        {{GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F}},
    }};

    constexpr static std::array<GLenum, PT_COUNT> glPixelTypeLUT = {
        {GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT}};

    constexpr static std::array<GLenum, C_COUNT> glChannelLUT = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
};