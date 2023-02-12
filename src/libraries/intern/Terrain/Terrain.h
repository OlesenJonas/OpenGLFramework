#pragma once

#include "CBTGPU.h"
#include <intern/Texture/ImageData.h>

class Terrain
{
  public:
    Terrain(
        Texture&& heightmap,
        Texture&& normalmap,
        Texture&& materialIDmap,
        uint8_t materialsToAllocate,
        int materialTextureWidth,
        int materialTextureHeight,
        Framebuffer& targetFramebuffer);

    bool setMaterial(
        uint8_t index,
        ImageData diffuseData,
        ImageData normalData,
        ImageData ordData,
        ImageData heightData,
        float materialDimensions);
    void finalizeMaterials();

    void update();
    void draw(Framebuffer& framebufferToWriteInto);

    void drawSettings();
    void updateSettingsBuffer();
    void drawTimers();

    struct Settings
    {
        float triplanarSharpness = 3.0f;
        float materialNormalIntensity = 0.7f;
        float materialDisplacementIntensity = 0.0f;
        int materialDisplacementLodOffset = 2;
        int visMode = 2;
    } settings;
    GLuint settingsBuffer;

    Texture heightmap;
    Texture normalmap;
    Texture materialIDmap;
    uint8_t numberOfMaterialsAllocated = 0;
    int textureArrayWidth = 0;
    int textureArrayHeight = 0;
    GLuint diffuseTextureArray;
    GLuint normalTextureArray;
    GLuint ordTextureArray;
    GLuint heightTextureArray;
    std::vector<float> textureScales;
    GLuint textureArrayInfoSSBO;

    CBTGPU cbt;
};