#pragma once

#include "../Mesh.h"

#include <string_view>

namespace OBJ
{
    std::vector<VertexStruct> load(const std::string& path);
} // namespace OBJ