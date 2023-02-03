#pragma once

#include "Texture.h"

#include <memory>

// struct containing image data + all necessary metadata to read/upload it
struct ImageData
{
    uint32_t width = 0;
    uint32_t height = 0;
    Texture::Channels channels = Texture::Channels::R;
    Texture::PixelType pixelType = Texture::PixelType::UCHAR_SRGB;
    std::unique_ptr<unsigned char[]> data;
};