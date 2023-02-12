#include "Terrain.h"
#include "Context/Context.h"
#include "Framebuffer/Framebuffer.h"
#include "Terrain/Terrain.h"

#include <ImGui/imgui.h>

Terrain::Terrain(
    Texture&& heightmap,
    Texture&& normalmap,
    Texture&& materialIDmap,
    uint8_t materialsToAllocate,
    int materialTextureWidth,
    int materialTextureHeight,
    Framebuffer& targetFramebuffer)
    : numberOfMaterialsAllocated(materialsToAllocate),
      textureArrayWidth(materialTextureWidth),
      textureArrayHeight(materialTextureHeight),
      heightmap(std::move(heightmap)),
      normalmap(std::move(normalmap)),
      materialIDmap(std::move(materialIDmap)),
      cbt(*this, 25, *targetFramebuffer.getDepthTexture())
{
    glTextureParameteri(this->heightmap.getTextureID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(this->heightmap.getTextureID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    textureScales.resize(materialsToAllocate);

    GLfloat maxAniso = 0;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &diffuseTextureArray);
    glObjectLabel(GL_TEXTURE, diffuseTextureArray, -1, "Diffuse Texture Array");
    glTextureStorage3D(
        diffuseTextureArray, 8, GL_SRGB8, materialTextureWidth, materialTextureHeight, materialsToAllocate);
    glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(diffuseTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(diffuseTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(diffuseTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameterf(diffuseTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &normalTextureArray);
    glObjectLabel(GL_TEXTURE, normalTextureArray, -1, "Normal Texture Array");
    glTextureStorage3D(
        normalTextureArray, 8, GL_RGB8, materialTextureWidth, materialTextureHeight, materialsToAllocate);
    glTextureParameteri(normalTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(normalTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(normalTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(normalTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(normalTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameterf(normalTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &heightTextureArray);
    glObjectLabel(GL_TEXTURE, heightTextureArray, -1, "Height Texture Array");
    glTextureStorage3D(
        heightTextureArray, 8, GL_R8, materialTextureWidth, materialTextureHeight, materialsToAllocate);
    glTextureParameteri(heightTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(heightTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(heightTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(heightTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(heightTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameterf(heightTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &ordTextureArray);
    glObjectLabel(GL_TEXTURE, ordTextureArray, -1, "ORD Texture Array");
    glTextureStorage3D(
        ordTextureArray, 8, GL_RGB8, materialTextureWidth, materialTextureHeight, materialsToAllocate);
    glTextureParameteri(ordTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(ordTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(ordTextureArray, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(ordTextureArray, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(ordTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameterf(ordTextureArray, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    glCreateBuffers(1, &textureArrayInfoSSBO);
    glObjectLabel(GL_BUFFER, textureArrayInfoSSBO, -1, "Texture Array Info Buffer");
    glNamedBufferStorage(
        textureArrayInfoSSBO, textureScales.size() * sizeof(textureScales[0]), textureScales.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, textureArrayInfoSSBO);

    glCreateBuffers(1, &settingsBuffer);
    glObjectLabel(GL_BUFFER, settingsBuffer, -1, "Terrain settings buffer");
    glNamedBufferStorage(settingsBuffer, sizeof(settings), &settings, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, settingsBuffer);
}

bool Terrain::setMaterial(
    uint8_t index,
    ImageData diffuseData,
    ImageData normalData,
    ImageData ordData,
    ImageData heightData,
    float materialDimensions)
{
    assert(index < numberOfMaterialsAllocated);

    assert(diffuseData.channels == Texture::Channels::RGB);
    assert(diffuseData.pixelType == Texture::PixelType::UCHAR_SRGB);
    glTextureSubImage3D(
        diffuseTextureArray,
        0,
        0,
        0,
        index,
        textureArrayWidth,
        textureArrayHeight,
        1,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        diffuseData.data.get());
    assert(normalData.channels == Texture::Channels::RGB);
    assert(normalData.pixelType == Texture::PixelType::UCHAR);
    glTextureSubImage3D(
        normalTextureArray,
        0,
        0,
        0,
        index,
        textureArrayWidth,
        textureArrayHeight,
        1,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        normalData.data.get());
    assert(ordData.channels == Texture::Channels::RGB);
    assert(ordData.pixelType == Texture::PixelType::UCHAR);
    glTextureSubImage3D(
        ordTextureArray,
        0,
        0,
        0,
        index,
        textureArrayWidth,
        textureArrayHeight,
        1,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        ordData.data.get());
    assert(heightData.channels == Texture::Channels::R);
    assert(heightData.pixelType == Texture::PixelType::UCHAR);
    glTextureSubImage3D(
        heightTextureArray,
        0,
        0,
        0,
        index,
        textureArrayWidth,
        textureArrayHeight,
        1,
        GL_RED,
        GL_UNSIGNED_BYTE,
        heightData.data.get());

    textureScales[index] = materialDimensions;

    return true;
}

void Terrain::finalizeMaterials()
{
    glGenerateTextureMipmap(diffuseTextureArray);
    glGenerateTextureMipmap(normalTextureArray);
    glGenerateTextureMipmap(heightTextureArray);
    glGenerateTextureMipmap(ordTextureArray);

    glDeleteBuffers(1, &textureArrayInfoSSBO);
    glCreateBuffers(1, &textureArrayInfoSSBO);
    glObjectLabel(GL_BUFFER, textureArrayInfoSSBO, -1, "Texture Array Info Buffer");
    glNamedBufferStorage(
        textureArrayInfoSSBO, textureScales.size() * sizeof(textureScales[0]), textureScales.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, textureArrayInfoSSBO);
}

void Terrain::update()
{
    const Context& ctx = *Context::globalContext;
    glBindTextureUnit(0, heightmap.getTextureID());
    if(!cbt.getSettings().freezeUpdates)
    {
        const auto& cam = *ctx.camera;
        cbt.update(
            cam.getProjView(), {Context::globalContext->internalWidth, Context::globalContext->internalHeight});
    }
    cbt.doSumReduction();
    cbt.writeIndirectCommands();
}

void Terrain::draw(Framebuffer& framebufferToWriteInto)
{
    const auto& cam = *Context::globalContext->camera;
    cbt.draw(cam.getView(), cam.getProj(), framebufferToWriteInto);
    if(cbt.getSettings().drawOutline)
    {
        cbt.drawOutline(cam.getProjView());
    }
}

void Terrain::drawSettings()
{
    bool changed = false;
    changed |= ImGui::DragFloat("Triplanar blending sharpness", &settings.triplanarSharpness, 0.05f, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("Normal intensity", &settings.materialNormalIntensity, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Displacement intensity", &settings.materialDisplacementIntensity, 0.0f, 1.0f);
    changed |= ImGui::SliderInt("Displacement lod offset", &settings.materialDisplacementLodOffset, 0, 7);
    changed |= ImGui::Combo("Vis mode", &settings.visMode, "Nearest Patch\0Interpolated Patches\0Shaded\0");
    if(changed)
        updateSettingsBuffer();
    ImGui::Indent(5.0f);
    if(ImGui::CollapsingHeader("CBT##settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        cbt.drawUI();
    }
    ImGui::Indent(-5.0f);
}

void Terrain::updateSettingsBuffer()
{
    glNamedBufferSubData(settingsBuffer, 0, sizeof(settings), &settings);
}

void Terrain::drawTimers()
{
    ImGui::TextUnformatted("CBT");
    ImGui::Indent(15.0f);
    cbt.drawTimers();
    ImGui::Indent(-15.0f);
}