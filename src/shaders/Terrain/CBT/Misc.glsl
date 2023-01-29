#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#include "LowLevel.glsl"

/*
vec2[3] cornersOfLeftChild(const vec2[3] corners)
{
    return vec2[3](corners[0], 0.5f * (corners[0] + corners[2]), corners[1]);
}

vec2[3] cornersOfRightChild(const vec2[3] corners)
{
    return vec2[3](corners[1], 0.5f * (corners[0] + corners[2]), corners[2]);
}
*/
void cornersOfLeftChild(inout vec2[3] corners)
{
    corners = vec2[3](corners[0], 0.5f * (corners[0] + corners[2]), corners[1]);
}

void cornersOfRightChild(inout vec2[3] corners)
{
    corners = vec2[3](corners[1], 0.5f * (corners[0] + corners[2]), corners[2]);
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

/*
    returns the corners of a triangle,
        centered in a way that the "center square" of a triangle at subdivision level 2
        has dimensions [-0.5,0.5]^2
*/
/*
vec2[3] cornersFromNode(const Node node)
{
    const vec2 p0 = vec2(-0.5f, -1.5f);
    const vec2 p1 = vec2(-0.5f,  0.5f);
    const vec2 p2 = vec2( 1.5f,  0.5f);
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

    // tri subdivision used in paper has the disadvantege of flipping the winding order every level
    if((node.depth & 1u) != 0u)
    {
        corners = vec2[3](corners[2], corners[1], corners[0]);
    }

    return corners;
}
*/

/*
    returns the corners of a triangle,
        centered in a way that the "center square" of a triangle at subdivision level 2
        has dimensions [-0.5,0.5]^2
*/
vec2[3] cornersFromNode(const Node node)
{
    const vec2 p0 = vec2(-0.5f, -1.5f);
    const vec2 p1 = vec2(-0.5f,  0.5f);
    const vec2 p2 = vec2( 1.5f,  0.5f);
    vec2[3] corners = vec2[3](p0, p1, p2);
    for(int bitID = int(node.depth) - 1; bitID >= 0; bitID--)
    {
        if(getSingleBitValue(node.heapIndex, bitID) == 0u)
        {
            cornersOfLeftChild(corners);
        }
        else
        {
            cornersOfRightChild(corners);
        }
    }

    // tri subdivision used in paper has the disadvantege of flipping the winding order every level
    if((node.depth & 1u) != 0u)
    {
        corners = vec2[3](corners[2], corners[1], corners[0]);
    }

    return corners;
}

/*
    returns the corners of a triangle,
        centered in a way that the "center square" of a triangle at subdivision level 2
        has dimensions [-0.5,0.5]^2
*/
// Using matrices as outlined in the paper actually just seems to be less efficient then passing the corners and
// splitting "by hand" (seems reasonable since matrix mult does a lot more math ops than really needed)
// maybe passing vec2[3] around is worse on other hardware but atm for me it seems more efficient!
vec2[3] cornersFromNodeMatrix(const Node node)
{
    //todo: could tranpose everything so that accessing the rows is easier
    //      would need to change order of matrix mult also!
    const vec2 p0 = vec2(-0.5f, -1.5f);
    const vec2 p1 = vec2(-0.5f,  0.5f);
    const vec2 p2 = vec2( 1.5f,  0.5f);
    mat2x3 cornerMat = mat2x3(
        //1st column
        p0.x, p1.x, p2.x,
        //2nd column
        p0.y, p1.y, p2.y
    );
    mat3 splitMat = mat3(
        //1st column
        0, 0.5, 0,
        //2nd column
        0, 0, 0,
        //3rd column
        0, 0.5, 0
    );
    for(int bitID = int(node.depth) - 1; bitID >= 0; bitID--)
    {
        uint splitDirection = getSingleBitValue(node.heapIndex, bitID);
        
        //set matrix entries depending on which side split
        splitMat[0][0] = 1-splitDirection;
        
        splitMat[1][0] = splitDirection;
        splitMat[1][2] = 1-splitDirection;

        splitMat[2][2] = splitDirection;

        cornerMat = splitMat * cornerMat;
    }

    // tri subdivision used in paper has the disadvantege of flipping the winding order every level
    if((node.depth & 1u) != 0u)
    {
        return vec2[3](
            vec2(cornerMat[0][2], cornerMat[1][2]),
            vec2(cornerMat[0][1], cornerMat[1][1]),
            vec2(cornerMat[0][0], cornerMat[1][0]));
    }

    return vec2[3](
            vec2(cornerMat[0][0], cornerMat[1][0]),
            vec2(cornerMat[0][1], cornerMat[1][1]),
            vec2(cornerMat[0][2], cornerMat[1][2]));
}

void centerAndScaleCorners(inout vec2[3] corners, float scale)
{
    #pragma optionNV (unroll all)
    for(int i=0; i<3; i++)
    {
        corners[i] *= scale;
    }
}