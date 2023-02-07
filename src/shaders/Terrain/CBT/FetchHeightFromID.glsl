#ifndef FETCHFROMID_GLSL
#define FETCHFROMID_GLSL

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded
#endif

float getFlatHeightFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos)
{
    const float texScale = textureScales[materialID];
    worldPos = worldPos/texScale;
    return textureLod(heightArray, vec3(worldPos.xz, materialID), materialDisplacementLodOffset).r;
}

float getBiplanarHeightFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    
    const vec2 flipFactorXY = vec2(-sign(macroNormalTS.y), 1);
    const vec2 flipFactorZY = vec2(-sign(macroNormalTS.x), 1);
    
    const vec2 uvXY = worldPos.xy*flipFactorXY/ texScale;
    const vec2 uvZY = worldPos.zy*flipFactorZY/ texScale;

    const float displacementXY = textureLod(heightArray, vec3(uvXY, materialID), materialDisplacementLodOffset).r;
    const float displacementZY = textureLod(heightArray, vec3(uvZY, materialID), materialDisplacementLodOffset).r;

    vec2 weights = pow(abs(macroNormalTS.xy),vec2(triplanarSharpness));
    weights /= weights.x+weights.y;

    return mix(displacementXY, displacementZY, weights.x);
}

float getHeightFromTexelAndWorldPos(const ivec2 texelPos, const vec3 worldPos, const vec3 macroNormalTS)
{
    uint materialID = texelFetch(materialIDTex, texelPos, 0).r;
    bool isTriplanar = (materialID & 0x80) > 0;
    materialID = materialID & 0x7F;
    if(isTriplanar)
        return getBiplanarHeightFromTexelAndWorldPos(materialID, texelPos, worldPos, macroNormalTS);
    else
        return getFlatHeightFromTexelAndWorldPos(materialID, texelPos, worldPos);
}

#endif