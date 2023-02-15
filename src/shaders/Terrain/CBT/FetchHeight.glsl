#ifndef FETCHFROMID_GLSL
#define FETCHFROMID_GLSL

#ifdef GLSLANGVALIDATOR
    // otherwise glslangValidator complains about #include
    #extension GL_GOOGLE_include_directive : require
    //for OpenGL those will be parsed when shader is loaded
    #ifndef ONLY_FUNCTIONS
        uniform sampler2DArray heightArray;
        uniform usampler2D materialIDTex;
        layout(std430, binding = 3) buffer textureInfoBuffer
        {   
            float textureScales[];
        };
        #include "../SettingsStruct.glsl"
        layout(binding = 4) uniform terrainSettingsBuffer
        {
            TerrainSettings terrainSettings;
        };
    #endif
#endif

float flatSampleOfHeightFromID(const uint materialID, vec2 uv)
{
    const float texScale = textureScales[materialID];
    uv = uv/texScale;
    return textureLod(heightArray, vec3(uv, materialID), terrainSettings.materialDisplacementLodOffset).r;
}

float biplanarSampleOfHeightFromID(const uint materialID, vec2 uvXY, vec2 uvZY, const float weight)
{
    const float texScale = textureScales[materialID];

    uvXY /= texScale;
    uvZY /= texScale;

    const float displacementXY = textureLod(heightArray, vec3(uvXY, materialID), terrainSettings.materialDisplacementLodOffset).r;
    const float displacementZY = textureLod(heightArray, vec3(uvZY, materialID), terrainSettings.materialDisplacementLodOffset).r;

    return mix(displacementXY, displacementZY, weight);
}

float getFlatHeightFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos)
{
    const float texScale = textureScales[materialID];
    worldPos = worldPos/texScale;
    return textureLod(heightArray, vec3(worldPos.xz, materialID), terrainSettings.materialDisplacementLodOffset).r;
}

float getBiplanarHeightFromTexelAndWorldPos(const uint materialID, const ivec2 texelPos, vec3 worldPos, const vec3 macroNormalTS)
{
    const float texScale = textureScales[materialID];
    
    const vec2 flipFactorXY = vec2(-sign(macroNormalTS.y), 1);
    const vec2 flipFactorZY = vec2(-sign(macroNormalTS.x), 1);
    
    const vec2 uvXY = worldPos.xy*flipFactorXY/ texScale;
    const vec2 uvZY = worldPos.zy*flipFactorZY/ texScale;

    const float displacementXY = textureLod(heightArray, vec3(uvXY, materialID), terrainSettings.materialDisplacementLodOffset).r;
    const float displacementZY = textureLod(heightArray, vec3(uvZY, materialID), terrainSettings.materialDisplacementLodOffset).r;

    vec2 weights = pow(abs(macroNormalTS.xy),vec2(terrainSettings.triplanarSharpness));
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