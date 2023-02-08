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
layout (location = 3) uniform float materialNormalIntensity = 0.7;
layout (location = 4) uniform mat4 viewMatrix;
uniform float triplanarSharpness = 3.0;

layout (location = 0) in vec3 worldPosNoDisplacement;
layout (location = 1) in vec4 projPosNoDisplacement;

void main()
{
    nonDisplacedPos = worldPosNoDisplacement;

    //need to explicitly calculate derivatives before scaling by textureScale, otherweise pixel neighbours
    //can have discontinuities in their UVs
    const vec3 dPdx = dFdx(worldPosNoDisplacement);
    const vec3 dPdy = dFdy(worldPosNoDisplacement);

    uint packeddX = packHalf2x16(vec2(dPdx.x, dPdy.x));
    uint packeddY = packHalf2x16(vec2(dPdx.y, dPdy.y));
    uint packeddZ = packHalf2x16(vec2(dPdx.z, dPdy.z));

    float nonDisplacedDepth = 0.5+0.5*(projPosNoDisplacement/projPosNoDisplacement.w).z;
    fragmentColor = vec4(uintBitsToFloat(packeddX), uintBitsToFloat(packeddY), uintBitsToFloat(packeddZ), nonDisplacedDepth);
    return;
}