#include <cassert>
#include <intern/Texture/Texture.h>

int main()
{
    using Channels = Texture::Channels;
    using PixelType = Texture::PixelType;
    {
        static_assert(Texture::getGLFormat(Channels::R, PixelType::UCHAR) == GL_R8);
        static_assert(Texture::getGLFormat(Channels::R, PixelType::USHORT) == GL_R16);
        static_assert(Texture::getGLFormat(Channels::R, PixelType::FLOAT) == GL_R32F);

        static_assert(Texture::getGLFormat(Channels::RG, PixelType::UCHAR) == GL_RG8);
        static_assert(Texture::getGLFormat(Channels::RG, PixelType::USHORT) == GL_RG16);
        static_assert(Texture::getGLFormat(Channels::RG, PixelType::FLOAT) == GL_RG32F);

        static_assert(Texture::getGLFormat(Channels::RGB, PixelType::UCHAR) == GL_RGB8);
        static_assert(Texture::getGLFormat(Channels::RGB, PixelType::USHORT) == GL_RGB16);
        static_assert(Texture::getGLFormat(Channels::RGB, PixelType::FLOAT) == GL_RGB32F);

        static_assert(Texture::getGLFormat(Channels::RGBA, PixelType::UCHAR) == GL_RGBA8);
        static_assert(Texture::getGLFormat(Channels::RGBA, PixelType::USHORT) == GL_RGBA16);
        static_assert(Texture::getGLFormat(Channels::RGBA, PixelType::FLOAT) == GL_RGBA32F);
    }
    {
        static_assert(Texture::getGLChannel(Channels::R) == GL_RED);
        static_assert(Texture::getGLChannel(Channels::RG) == GL_RG);
        static_assert(Texture::getGLChannel(Channels::RGB) == GL_RGB);
        static_assert(Texture::getGLChannel(Channels::RGBA) == GL_RGBA);
    }
    {
        static_assert(Texture::getGLPixelType(PixelType::UCHAR) == GL_UNSIGNED_BYTE);
        static_assert(Texture::getGLPixelType(PixelType::USHORT) == GL_UNSIGNED_SHORT);
        static_assert(Texture::getGLPixelType(PixelType::FLOAT) == GL_FLOAT);
    }
    return 0;
}
