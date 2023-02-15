#ifdef GLSLANGVALIDATOR
    #ifndef ONLY_DEFINES
    #ifndef DISPLACEMENT_ALREADY_DEFINED
        uniform sampler2D displacementTex;
    #endif
    #endif
#endif

#define TERRAIN_SIZE 500
#define TERRAIN_HEIGHT 50

#ifndef ONLY_DEFINES
vec4 transformFlatPointToWorldSpace(inout vec2 flatPosition)
{
    const vec2 uv = vec2(1,-1)*flatPosition + 0.5;

    const float heightValue = textureLod(displacementTex,uv,0).r;
    return vec4(
        TERRAIN_SIZE   * flatPosition.x,
        TERRAIN_HEIGHT * heightValue,
        TERRAIN_SIZE   * flatPosition.y,
        1.0
    );
}

vec4[3] transformFlatCornersToWorldCorners(inout vec2[3] flatCorners)
{
    return vec4[3](
        transformFlatPointToWorldSpace(flatCorners[0]),
        transformFlatPointToWorldSpace(flatCorners[1]),
        transformFlatPointToWorldSpace(flatCorners[2])
    );
}
#endif
