#include "TriangleTemplate.h"

#include <array>

TriangleTemplate::TriangleTemplate(uint8_t subDivLevel)
{
    // todo: just copy pasted from cube for now. Actually create triangle geo!
    std::vector<VertexStruct> vertices = {};
    vertices.reserve(6 * 4);
    // half size
    float hs = 0.5f * size;
    // front face
    vertices.push_back({.pos = {-hs, -hs, hs}, .nrm = {0, 0, 1}, .uv = {0, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, -hs, hs}, .nrm = {0, 0, 1}, .uv = {1, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, hs, hs}, .nrm = {0, 0, 1}, .uv = {1, 1}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {-hs, hs, hs}, .nrm = {0, 0, 1}, .uv = {0, 1}, .tang = {1, 0, 0, 1}});
    // back face
    vertices.push_back({.pos = {hs, -hs, -hs}, .nrm = {0, 0, -1}, .uv = {0, 0}, .tang = {-1, 0, 0, 1}});
    vertices.push_back({.pos = {-hs, -hs, -hs}, .nrm = {0, 0, -1}, .uv = {1, 0}, .tang = {-1, 0, 0, 1}});
    vertices.push_back({.pos = {-hs, hs, -hs}, .nrm = {0, 0, -1}, .uv = {1, 1}, .tang = {-1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, hs, -hs}, .nrm = {0, 0, -1}, .uv = {0, 1}, .tang = {-1, 0, 0, 1}});
    // right face
    vertices.push_back({.pos = {hs, -hs, hs}, .nrm = {1, 0, 0}, .uv = {0, 0}, .tang = {0, 0, -1, 1}});
    vertices.push_back({.pos = {hs, -hs, -hs}, .nrm = {1, 0, 0}, .uv = {1, 0}, .tang = {0, 0, -1, 1}});
    vertices.push_back({.pos = {hs, hs, -hs}, .nrm = {1, 0, 0}, .uv = {1, 1}, .tang = {0, 0, -1, 1}});
    vertices.push_back({.pos = {hs, hs, hs}, .nrm = {1, 0, 0}, .uv = {0, 1}, .tang = {0, 0, -1, 1}});
    // left face
    vertices.push_back({.pos = {-hs, -hs, -hs}, .nrm = {-1, 0, 0}, .uv = {0, 0}, .tang = {0, 0, 1, 1}});
    vertices.push_back({.pos = {-hs, -hs, hs}, .nrm = {-1, 0, 0}, .uv = {1, 0}, .tang = {0, 0, 1, 1}});
    vertices.push_back({.pos = {-hs, hs, hs}, .nrm = {-1, 0, 0}, .uv = {1, 1}, .tang = {0, 0, 1, 1}});
    vertices.push_back({.pos = {-hs, hs, -hs}, .nrm = {-1, 0, 0}, .uv = {0, 1}, .tang = {0, 0, 1, 1}});
    // top face
    vertices.push_back({.pos = {-hs, hs, hs}, .nrm = {0, 1, 0}, .uv = {0, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, hs, hs}, .nrm = {0, 1, 0}, .uv = {1, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, hs, -hs}, .nrm = {0, 1, 0}, .uv = {1, 1}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {-hs, hs, -hs}, .nrm = {0, 1, 0}, .uv = {0, 1}, .tang = {1, 0, 0, 1}});
    // bottom face
    vertices.push_back({.pos = {-hs, -hs, -hs}, .nrm = {0, -1, 0}, .uv = {0, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, -hs, -hs}, .nrm = {0, -1, 0}, .uv = {1, 0}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {hs, -hs, hs}, .nrm = {0, -1, 0}, .uv = {1, 1}, .tang = {1, 0, 0, 1}});
    vertices.push_back({.pos = {-hs, -hs, hs}, .nrm = {0, -1, 0}, .uv = {0, 1}, .tang = {1, 0, 0, 1}});

    // index structure
    std::vector<GLuint> indices = {};
    indices.reserve(6 * 6);
    for(int i = 0; i < 6; i++)
    {
        indices.push_back(i * 4 + 0);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 3);

        indices.push_back(i * 4 + 3);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 2);
    }

    // TODO: MAKE INIT VIRTUAL AND OVERRIDE IN THIS CLASS! 100% ONLY NEED POSITION ATTRIBUTE!
    //       MAYBE UV LATER FOR SOME FANCY BARYCENTRIC EFFECT (BIG IF)
    init(vertices, indices);
}