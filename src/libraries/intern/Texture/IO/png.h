#pragma once

#include <string_view>

#include "../ImageData.h"

namespace png
{
    /*
        path needs to point to a null-terminated string
        Does not handle bit depths != 8 or 16
    */
    ImageData read(std::string_view path, bool flip = false);
} // namespace png