#pragma once

#include <intern/Mesh/Mesh.h>

#include <array>

class TriangleTemplate : public Mesh
{
  public:
    explicit TriangleTemplate(uint8_t subDivLevel);
};