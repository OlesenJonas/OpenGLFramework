#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#include "LowLevel.glsl"

vec2[3] cornersOfLeftChild(const vec2[3] corners)
{
    return vec2[3](corners[0], 0.5f * (corners[0] + corners[2]), corners[1]);
}

vec2[3] cornersOfRightChild(const vec2[3] corners)
{
    return vec2[3](corners[1], 0.5f * (corners[0] + corners[2]), corners[2]);
}

bool pointInTriangle(const vec2 pos, const vec2[3] corners)
{
    /* unoptimized but readable */

    const vec2 edge0 = corners[1] - corners[0];
    const vec2 nrm0 = vec2(-edge0.y, edge0.x);
    const bool inside1 = dot(pos - corners[0], nrm0) < 0.0;
    // if(dot(pos - corners[0], nrm0) > 0.0)
    // return false;

    const vec2 edge1 = corners[2] - corners[1];
    const vec2 nrm1 = vec2(-edge1.y, edge1.x);
    const bool inside2 = dot(pos - corners[1], nrm1) < 0.0;
    // if(dot(pos - corners[1], nrm1) > 0.0)
    // return false;

    const vec2 edge2 = corners[0] - corners[2];
    const vec2 nrm2 = vec2(-edge2.y, edge2.x);
    const bool inside3 = dot(pos - corners[2], nrm2) < 0.0;
    // if(dot(pos - corners[2], nrm2) > 0.0)
    // return false;

    // this test works for triangles of either winding order
    return (inside1 == inside2 && inside2 == inside3);
    // return true;
}

vec2[3] cornersFromNode(const Node node)
{
    const vec2 p0 = vec2(-1.0f, -1.0f);
    const vec2 p1 = vec2(-1.0f, 1.0f);
    const vec2 p2 = vec2(1.0f, 1.0f);
    vec2[3] corners = vec2[3](p0, p1, p2);
    for(int bitID = int(node.depth) - 1; bitID >= 0; bitID--)
    {
        if(getSingleBitValue(node.heapIndex, bitID) == 0u)
        {
            corners = cornersOfLeftChild(corners);
        }
        else
        {
            corners = cornersOfRightChild(corners);
        }
    }

    return corners;
}