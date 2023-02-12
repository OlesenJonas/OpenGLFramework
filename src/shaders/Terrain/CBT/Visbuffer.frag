#version 430

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

layout(early_fragment_tests) in;

layout (location = 0) out vec4 fragmentColor;
layout (location = 1) out vec3 nonDisplacedPos;

layout (location = 0) uniform mat4 projectionViewMatrix;
layout (location = 4) uniform mat4 viewMatrix;

layout (location = 0) in vec3 worldPosNoDisplacement;

#include "../SettingsStruct.glsl"
layout(binding = 4) uniform terrainSettingsBuffer
{
    TerrainSettings terrainSettings;
};

void main()
{
    nonDisplacedPos = worldPosNoDisplacement;

    const vec3 dPdx = dFdx(worldPosNoDisplacement);
    const vec3 dPdy = dFdy(worldPosNoDisplacement);

    uint packeddX = packHalf2x16(vec2(dPdx.x, dPdy.x));
    uint packeddY = packHalf2x16(vec2(dPdx.y, dPdy.y));
    uint packeddZ = packHalf2x16(vec2(dPdx.z, dPdy.z));

    //could optimize storage of non-displaced position by storing linear depth in w
    //and "uv" in another rendertarget, then simply interpolate from frustum corners
    fragmentColor = vec4(uintBitsToFloat(packeddX), uintBitsToFloat(packeddY), uintBitsToFloat(packeddZ), 0.0);
    return;
}