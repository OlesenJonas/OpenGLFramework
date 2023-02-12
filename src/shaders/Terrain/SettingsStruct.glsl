#ifndef TERRAIN_SETTINGSSTRUCT_GLSL
#define TERRAIN_SETTINGSSTRUCT_GLSL

struct TerrainSettings
{
    float triplanarSharpness;
    float materialNormalIntensity;
    float materialDisplacementIntensity;
    int materialDisplacementLodOffset;
    int visMode;
};

#endif