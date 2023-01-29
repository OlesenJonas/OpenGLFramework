#include "Texture/ImageData.h"
#include <libpng16/png.h>
#include <libpng16/pngconf.h>

#include <intern/Misc/sRGB.h>
#include <intern/Texture/IO/png.h>

#include <array>
#include <cstddef>
#include <vector>

ImageData png::read(std::string_view path, bool imageIsInSRGB, bool flip)
{
    // todo: on errors dont just return {}, at least print why error happened

    /*
        Code taken from the libpng docs
    */

    ImageData imageData;

    /* open file */
    FILE* fp = fopen(path.data(), "rb");
    if(fp == nullptr)
    {
        assert(false && "Cant open file");
        return {};
    }
    std::array<png_byte, 8> header{};
    fread(header.data(), 1, 8, fp);
    if(png_sig_cmp(header.data(), 0, 8) != 0) // 0 return means file is png
    {
        assert(false && "File isnt a png");
        return {};
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if(png_ptr == nullptr)
    {
        return {};
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == nullptr)
    {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return {};
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if(end_info == nullptr)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return {};
    }

    // todo: could make this construct a macro since its called multiple times
    if(setjmp(png_jmpbuf(png_ptr))) // NOLINT
    {
        // error during init of file
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return {};
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8); // not sure if this has to go before init_io()

    // todo: handle unknown chunks? (png_set_read_user_chunk_fn())

    // todo: could setup a callback to allow for progress tracking
    //  void read_row_callback(png_ptr ptr, png_uint_32 row, int pass);
    //  {
    //      /* put your code here */
    //  }
    // png_set_read_status_fn(png_ptr, write_row_callback);

    png_set_user_limits(png_ptr, 8192, 8192);

    /* read header */
    if(setjmp(png_jmpbuf(png_ptr))) // NOLINT
    {
        // error during reading output info
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return {};
    }

    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = 0;
    png_uint_32 height = 0;
    png_int_32 bit_depth = 0;
    png_int_32 color_type = 0;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);
    imageData.width = static_cast<int>(width);
    imageData.height = static_cast<int>(height);

    // int srgbIntent = 0;
    // png_get_sRGB(png_ptr, info_ptr, &srgbIntent);
    // havent come across any png that actually has this flag set
    // so just let the user decide if the data is supposed to be in sRGB space
    int srgbIntent = static_cast<int>(imageIsInSRGB);

    bool transform16BitToLinear = false;
    switch(bit_depth)
    {
    case 8:
        imageData.pixelType = srgbIntent != 0 ? Texture::PixelType::UCHAR_SRGB : Texture::PixelType::UCHAR;
        break;
    case 16:
        imageData.pixelType = Texture::PixelType::USHORT;
        // disable srgb on 16bit images, since its not supported by OpenGL.
        //       manually transform all pixels sRGB -> linear RGB before uploading to GPU
        // todo: warning with ^^ message
        if(srgbIntent != 0)
        {
            transform16BitToLinear = true;
        }
        break;
    default:
        // unknown/unsupported pixel type
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        // just returning for now, but could also use pngs transforms to transform into known format
        return {};
    }

    switch(color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        imageData.channels = Texture::Channels::R;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        imageData.channels = Texture::Channels::RG;
        break;
    case PNG_COLOR_TYPE_RGB:
        imageData.channels = Texture::Channels::RGB;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        imageData.channels = Texture::Channels::RGBA;
        break;
    default:
        // unknown/unsupported color type
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        return {};
    }

    if(imageData.pixelType == Texture::PixelType::UCHAR_SRGB &&
       (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
    {
        // to support opengl texture formats, srgb textures with image data <= 2 channels will be padded to 3
        // nb: not sure what happended to GL_SLUMINANCE / GL_SLUMINANCE_ALPHA after the luminance formats were
        // deprecated. Those are mentioned in the srgb extension as 1/2 channel srgb textures
        imageData.channels = Texture::Channels::RGB;
        png_set_gray_to_rgb(png_ptr);
    }

    // byte order issue between file format and system
    if(bit_depth > 8)
    {
        png_set_swap(png_ptr);
    }

    /* read bytes */
    if(setjmp(png_jmpbuf(png_ptr))) // NOLINT
    {
        // error during reading image data
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        return {};
    }

    uint8_t bytesPerPixel = bit_depth / 8;
    uint8_t channelAmnt = 0;
    switch(imageData.channels)
    {
    case Texture::Channels::R:
        channelAmnt = 1;
        break;
    case Texture::Channels::RG:
        channelAmnt = 2;
        break;
    case Texture::Channels::RGB:
        channelAmnt = 3;
        break;
    case Texture::Channels::RGBA:
        channelAmnt = 4;
        break;
    default:
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return {};
    }
    bytesPerPixel *= channelAmnt;

    const size_t bytesInRow = static_cast<size_t>(bytesPerPixel) * width;
    imageData.data = std::make_unique<png_byte[]>(bytesInRow * height);
    static_assert(std::is_same_v<unsigned char, png_byte>);
    std::vector<png_byte*> row_pointers(height);
    for(png_int_32 i = 0; i < height; i++)
    {
        uint32_t rowIndex = flip ? height - i - 1 : i;
        row_pointers[rowIndex] = &imageData.data[i * bytesInRow];
    }
    png_read_image(png_ptr, row_pointers.data());

    // todo: make this whole section less ugly
    if(transform16BitToLinear)
    {
        auto* iter = reinterpret_cast<uint16_t*>(imageData.data.get());
        if(channelAmnt == 4)
        {
            for(auto i = 0; i < width * height; i++)
            {
                glm::vec3 pixel{float(iter[0]) / 65535, float(iter[1]) / 65535, float(iter[2]) / 65535};
                pixel = sRGBtoLinear(pixel);
                iter[0] = static_cast<uint16_t>(pixel.r * 65535);
                iter[1] = static_cast<uint16_t>(pixel.g * 65535);
                iter[2] = static_cast<uint16_t>(pixel.b * 65535);
                iter += 4; // NOLINT
            }
        }
        else if(channelAmnt == 3)
        {
            for(auto i = 0; i < width * height; i++)
            {
                glm::vec3 pixel{float(iter[0]) / 65535, float(iter[1]) / 65535, float(iter[2]) / 65535};
                pixel = sRGBtoLinear(pixel);
                iter[0] = static_cast<uint16_t>(pixel.r * 65535);
                iter[1] = static_cast<uint16_t>(pixel.g * 65535);
                iter[2] = static_cast<uint16_t>(pixel.b * 65535);
                iter += 3; // NOLINT
            }
        }
        else if(channelAmnt == 2)
        {
            for(auto i = 0; i < width * height; i++)
            {
                glm::vec2 pixel{float(iter[0]) / 65535, float(iter[1]) / 65535};
                pixel = sRGBtoLinear(pixel);
                iter[0] = static_cast<uint16_t>(pixel.r * 65535);
                iter[1] = static_cast<uint16_t>(pixel.g * 65535);
                iter += 2; // NOLINT
            }
        }
        else if(channelAmnt == 1)
        {
            for(auto i = 0; i < width * height; i++)
            {
                float pixel = float(iter[0]) / 65535;
                pixel = sRGBtoLinear(pixel);
                iter[0] = static_cast<uint16_t>(pixel * 65535);
                iter += 1; // NOLINT
            }
        }
    }

    /* end read */
    if(setjmp(png_jmpbuf(png_ptr))) // NOLINT
    {
        // error during end of read
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        return {};
    }

    png_read_end(png_ptr, end_info);

    /* cleanup allocation */
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    fclose(fp);

    return imageData;
}
