#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded  
#endif

#ifndef BLENDING_GLSL
#define BLENDING_GLSL

#include "MaterialAttributes.glsl"

float heightblend(const float input1, const float height1, const float input2, const float height2, const float heightblendFactor)
{
    float height_start = max(height1, height2) - heightblendFactor;
    float level1 = max(height1 - height_start, 0);
    float level2 = max(height2 - height_start, 0);
    return ((input1 * level1) + (input2 * level2)) / (level1 + level2);
}
vec3 heightblend(const vec3 input1, const float height1, const vec3 input2, const float height2, const float heightblendFactor)
{
    float height_start = max(height1, height2) - heightblendFactor;
    float level1 = max(height1 - height_start, 0);
    float level2 = max(height2 - height_start, 0);
    return ((input1 * level1) + (input2 * level2)) / (level1 + level2);
}
MaterialAttributes heightblend(const MaterialAttributes input1, const float height1, const MaterialAttributes input2, const float height2, const float heightblendFactor)
{
    float height_start = max(height1, height2) - heightblendFactor;
    float level1 = max(height1 - height_start, 0);
    float level2 = max(height2 - height_start, 0);
    return divide(
                add(multiply(input1, level1),
                    multiply(input2, level2)),
            level1 + level2);
}

float heightlerp(const float input1, const float height1, const float input2, const float height2, const float t, const float heightblendFactor)
{
    return heightblend(input1, height1 * (1 - t), input2, height2 * t, heightblendFactor);
}
vec3 heightlerp(const vec3 input1, const float height1, const vec3 input2, const float height2, const float t, const float heightblendFactor)
{
    return heightblend(input1, height1 * (1 - t), input2, height2 * t, heightblendFactor);
}
MaterialAttributes heightlerp(const MaterialAttributes input1, const float height1, const MaterialAttributes input2, const float height2, const float t, float heightblendFactor)
{
    return heightblend(input1, height1 * (1 - t), input2, height2 * t, heightblendFactor);
}

#endif