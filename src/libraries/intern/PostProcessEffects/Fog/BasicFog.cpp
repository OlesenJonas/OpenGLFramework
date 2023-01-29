#include "BasicFog.h"
#include "PostProcessEffects/Fog/BasicFog.h"

#include <ImGui/imgui.h>

#include <intern/Context/Context.h>
#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Misc/ImGuiExtensions.h>
#include <intern/PostProcessEffects/Fog/BasicFog.h>

BasicFogEffect::BasicFogEffect(int width, int height)
    : framebuffer(width, height, {{.internalFormat = GL_RGBA16F}}, false),
      shader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/BasicFog.frag"}}
{
    updateSettings();
};

const Texture& BasicFogEffect::execute(const Texture& colorInput, const Texture& depthInput)
{
    framebuffer.bind();
    // happens in post process pass atm, where depth test has already been disabled
    // glDisable(GL_DEPTH_TEST);

    glBindTextureUnit(0, colorInput.getTextureID());
    glBindTextureUnit(1, depthInput.getTextureID());
    shader.useProgram();
    const auto& cam = *Context::globalContext->getCamera();
    glUniform3fv(0, 1, glm::value_ptr(cam.getPosition()));
    const glm::mat4 invProjView = glm::inverse(*cam.getProj() * *cam.getView());
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(invProjView));
    fullScreenTri.draw();

    return framebuffer.getColorTextures()[0];
}

BasicFogEffect::Settings& BasicFogEffect::getSettings()
{
    return settings;
}
void BasicFogEffect::updateSettings()
{
    shader.useProgram();
    glUniform4fv(2, 1, glm::value_ptr(settings.absorptionCoefficient));
    glUniform4fv(3, 1, glm::value_ptr(settings.scatteringCoefficient));
    glUniform4fv(8, 1, glm::value_ptr(settings.extinctionCoefficient));
    glUniform4fv(4, 1, glm::value_ptr(settings.inscatteredLight));
    glUniform1f(5, settings.falloff);
    glUniform1f(6, settings.heightOffset);
    glUniform1i(7, settings.mode);
}

void BasicFogEffect::drawUI()
{
    if(ImGui::DragFloat("Falloff", &settings.falloff, 0.01f, .01f, FLT_MAX))
    {
        updateSettings();
    }
    if(ImGui::DragFloat("Height offset", &settings.heightOffset, 0.05f))
    {
        updateSettings();
    }
    if(ImGui::SliderInt("Fog color mode", &settings.mode, 0, 1))
    {
        updateSettings();
    }
    ImGui::SameLine();
    ImGui::Extensions::HelpMarker("0 = 1-transmittance, 1 = inscatter integral");
    ImGui::Separator();
    if(settings.mode == 0)
    {
        if(ImGui::ColorEdit3(
               "Extinction coefficient", &settings.extinctionCoefficient.x, ImGuiColorEditFlags_Float))
        {
            updateSettings();
        }
        if(ImGui::DragFloat(
               "Multiplier##extinction", &settings.extinctionCoefficient.w, 0.05f, 0.0f, FLT_MAX))
        {
            updateSettings();
        }
    }
    else if(settings.mode == 1)
    {
        const float absorptionCoeffLimit =
            length(settings.scatteringCoefficient * settings.scatteringCoefficient.w) == 0.0f ? .01f : 0.0f;
        const float scatteringCoeffLimit =
            length(settings.absorptionCoefficient * settings.absorptionCoefficient.w) == 0.0f ? .01f : 0.0f;
        if(ImGui::ColorEdit3(
               "Absorption coefficient", &settings.absorptionCoefficient.x, ImGuiColorEditFlags_Float))
        {
            settings.absorptionCoefficient = glm::max(settings.absorptionCoefficient, absorptionCoeffLimit);
            updateSettings();
        }
        if(ImGui::DragFloat(
               "Multiplier##absorption",
               &settings.absorptionCoefficient.w,
               0.05f,
               absorptionCoeffLimit,
               FLT_MAX))
        {
            updateSettings();
        }
        ImGui::Separator();
        if(ImGui::ColorEdit3(
               "Scattering coefficient", &settings.scatteringCoefficient.x, ImGuiColorEditFlags_Float))
        {
            settings.scatteringCoefficient = glm::max(settings.scatteringCoefficient, scatteringCoeffLimit);
            updateSettings();
        }
        if(ImGui::DragFloat(
               "Multiplier##scattering",
               &settings.scatteringCoefficient.w,
               0.05f,
               scatteringCoeffLimit,
               FLT_MAX))
        {
            updateSettings();
        }
    }
    ImGui::Separator();
    if(ImGui::ColorEdit3("Inscattered Light", &settings.inscatteredLight.x, ImGuiColorEditFlags_Float))
    {
        updateSettings();
    }
    if(ImGui::DragFloat("Multiplier##inscattering", &settings.inscatteredLight.w, 0.05f, .0f, FLT_MAX))
    {
        updateSettings();
    }
}