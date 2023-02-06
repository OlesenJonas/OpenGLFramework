#include "SSMSFog.h"
#include "Texture/Texture.h"

#include <ImGui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <initializer_list>
#include <string>

#include <intern/Context/Context.h>
#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Misc/ImGuiExtensions.h>
#include <intern/PostProcessEffects/Fog/BasicFog.h>

SSMSFogEffect::SSMSFogEffect(int width, int height, uint8_t levels)
    : levels(levels), width(width), height(height),
      directLight{
          {.name = "Direct light",
           .levels = 1,
           .width = width,
           .height = height,
           .internalFormat = GL_RGBA16F,
           .wrapS = GL_CLAMP_TO_EDGE,
           .wrapT = GL_CLAMP_TO_EDGE}},
      initialFogShader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/SSMSinitialFog.frag"}},
      downsample0to1Shader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/SSMSdownsample0to1.frag"}},
      downsampleShader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/SSMSdownsample.frag"}},
      upsampleShader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/SSMSupsample.frag"}},
      upsampleAndCombineShader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/SSMSupsampleAndCombine.frag"}}
{
    upsampleTextures.reserve(levels);
    downsampleTextures.reserve(levels);
    downsampleFramebuffers.reserve(levels);
    upsampleFramebuffers.reserve(levels);

    int currentWidth = width;
    int currentHeight = height;
    for(uint8_t i = 0; i < levels; i++)
    {
        const auto& currentDownsampleTex = downsampleTextures.emplace_back(TextureDesc{
            .name = ("Downsample tex level " + std::to_string(i)).c_str(),
            .levels = 1,
            .width = currentWidth,
            .height = currentHeight,
            .internalFormat = GL_RGBA16F,
            .wrapS = GL_CLAMP_TO_EDGE,
            .wrapT = GL_CLAMP_TO_EDGE});
        const auto& currentUpsampleTex = upsampleTextures.emplace_back(TextureDesc{
            .name = ("Upsample tex level " + std::to_string(i)).c_str(),
            .levels = 1,
            .width = currentWidth,
            .height = currentHeight,
            .internalFormat = GL_RGBA16F,
            .wrapS = GL_CLAMP_TO_EDGE,
            .wrapT = GL_CLAMP_TO_EDGE});
        downsampleFramebuffers.emplace_back(
            currentWidth,
            currentHeight,
            std::initializer_list<std::pair<const GLTexture&, int>>{{currentDownsampleTex, 0}},
            false);
        upsampleFramebuffers.emplace_back(
            currentWidth,
            currentHeight,
            std::initializer_list<std::pair<const GLTexture&, int>>{{currentUpsampleTex, 0}},
            false);
        currentWidth /= 2;
        currentHeight /= 2;
    }
    initialFogFramebuffer = Framebuffer{width, height, {{directLight, 0}, {downsampleTextures[0], 0}}, false};
    settings.steps = levels - 1;
    updateSettings();
};

const Texture& SSMSFogEffect::execute(const Texture& colorInput, const Texture& depthInput)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SSMSFog execute");

    // happens in post process pass atm, where depth test has already been disabled
    // glDisable(GL_DEPTH_TEST);

    const auto& cam = *Context::globalContext->getCamera();
    const glm::mat4 invProjView = glm::inverse(*cam.getProj() * *cam.getView());

    const int steps = int(settings.steps);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Initial fog");
    initialFogFramebuffer.bind();
    glBindTextureUnit(0, colorInput.getTextureID());
    glBindTextureUnit(1, depthInput.getTextureID());
    initialFogShader.useProgram();
    glUniform3fv(0, 1, glm::value_ptr(cam.getPosition()));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(invProjView));
    fullScreenTri.draw();
    glPopDebugGroup();

    // constructing mip pyramid
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Downscale mip pyramid");
    downsampleFramebuffers[1].bind();
    glBindTextureUnit(0, downsampleTextures[0].getTextureID());
    downsample0to1Shader.useProgram();
    fullScreenTri.draw();
    for(int level = 2; level <= steps; level++)
    {
        downsampleFramebuffers[level].bind();
        glBindTextureUnit(0, downsampleTextures[level - 1].getTextureID());
        fullScreenTri.draw();
    }
    glPopDebugGroup();

    // upscale and recombine
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Upscale and recombine mip pyramid");
    // combining the lowest two levels first (both downsample textures)
    upsampleShader.useProgram();

    upsampleFramebuffers[steps - 1].bind();
    glBindTextureUnit(0, downsampleTextures[steps - 1].getTextureID());
    glBindTextureUnit(1, downsampleTextures[steps].getTextureID());
    fullScreenTri.draw();
    // Then merge the downscale texture of each level, with the last combined upscale texture of the level
    // below
    for(int level = (steps - 1) - 1; level >= 1; level--)
    {
        upsampleFramebuffers[level].bind();
        glBindTextureUnit(0, downsampleTextures[level].getTextureID());
        glBindTextureUnit(1, upsampleTextures[level + 1].getTextureID());
        fullScreenTri.draw();
    }
    // last upsample also combines with original image
    upsampleFramebuffers[0].bind();
    upsampleAndCombineShader.useProgram();
    glBindTextureUnit(0, directLight.getTextureID());
    glBindTextureUnit(1, upsampleTextures[1].getTextureID());
    fullScreenTri.draw();

    glPopDebugGroup();

    glPopDebugGroup();
    return upsampleTextures[0];
}

SSMSFogEffect::Settings& SSMSFogEffect::getSettings()
{
    return settings;
}
void SSMSFogEffect::updateSettings()
{
    initialFogShader.useProgram();
    glUniform4fv(
        glGetUniformLocation(initialFogShader.getProgramID(), "absorptionCoefficient"),
        1,
        glm::value_ptr(settings.absorptionCoefficient));
    glUniform4fv(
        glGetUniformLocation(initialFogShader.getProgramID(), "scatteringCoefficient"),
        1,
        glm::value_ptr(settings.scatteringCoefficient));
    glUniform4fv(
        glGetUniformLocation(initialFogShader.getProgramID(), "inscatteredLight"),
        1,
        glm::value_ptr(settings.inscatteredLight));
    glUniform1f(glGetUniformLocation(initialFogShader.getProgramID(), "falloff"), settings.falloff);
    glUniform1f(glGetUniformLocation(initialFogShader.getProgramID(), "heightOffset"), settings.heightOffset);
    glUniform3fv(
        glGetUniformLocation(initialFogShader.getProgramID(), "blurTint"),
        1,
        glm::value_ptr(settings.blurTint));

    const float fracSteps = glm::fract(settings.steps);

    upsampleShader.useProgram();
    glUniform1f(0, 1.0f + fracSteps);
    glUniform1f(1, settings.blurWeight);

    upsampleAndCombineShader.useProgram();
    glUniform1f(0, 1.0f + fracSteps);
    glUniform1f(1, settings.steps);
    glUniform1f(2, settings.intensity);
}

void SSMSFogEffect::drawUI()
{
    bool changed = false;
    changed |= ImGui::DragFloat("Falloff", &settings.falloff, 0.01f, .01f, FLT_MAX);
    changed |= ImGui::DragFloat("Height offset", &settings.heightOffset, 0.05f);

    ImGui::Separator();

    if(ImGui::ColorEdit3(
           "Absorption coefficient", &settings.absorptionCoefficient.x, ImGuiColorEditFlags_Float))
    {
        settings.absorptionCoefficient = glm::max(settings.absorptionCoefficient, .001f);
        changed = true;
    }
    changed |=
        ImGui::DragFloat("Multiplier##absorption", &settings.absorptionCoefficient.w, 0.05f, .001f, FLT_MAX);

    ImGui::Separator();
    if(ImGui::ColorEdit3(
           "Scattering coefficient", &settings.scatteringCoefficient.x, ImGuiColorEditFlags_Float))
    {
        settings.scatteringCoefficient = glm::max(settings.scatteringCoefficient, .001f);
        changed = true;
    }
    changed |=
        ImGui::DragFloat("Multiplier##scattering", &settings.scatteringCoefficient.w, 0.05f, .001f, FLT_MAX);

    ImGui::Separator();
    changed |= ImGui::ColorEdit3(
        "Inscattered Light / Fog Color", &settings.inscatteredLight.x, ImGuiColorEditFlags_Float);
    changed |=
        ImGui::DragFloat("Multiplier##inscattering", &settings.inscatteredLight.w, 0.05f, .0f, FLT_MAX);

    // SSMS settings
    ImGui::Separator();
    ImGui::TextUnformatted("SSMS Parameters");
    changed |= ImGui::SliderFloat("Blur steps", &settings.steps, 2, levels - 1);
    changed |= ImGui::ColorEdit3("Blur Tint", &settings.blurTint.x, ImGuiColorEditFlags_Float);
    changed |= ImGui::SliderFloat(
        "Blur weight", &settings.blurWeight, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    changed |= ImGui::SliderFloat("Intensity", &settings.intensity, 0.0f, 1.0f);

    if(changed)
    {
        updateSettings();
    }
}