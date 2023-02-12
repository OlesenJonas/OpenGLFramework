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
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cam.getInvProjView()));
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
    glUniform4fv(
        glGetUniformLocation(shader.getProgramID(), "absorptionCoefficient"),
        1,
        glm::value_ptr(settings.absorptionCoefficient));
    glUniform4fv(
        glGetUniformLocation(shader.getProgramID(), "scatteringCoefficient"),
        1,
        glm::value_ptr(settings.scatteringCoefficient));
    glUniform4fv(
        glGetUniformLocation(shader.getProgramID(), "inscatteredLight"),
        1,
        glm::value_ptr(settings.inscatteredLight));
    glUniform1f(glGetUniformLocation(shader.getProgramID(), "falloff"), settings.falloff);
    glUniform1f(glGetUniformLocation(shader.getProgramID(), "heightOffset"), settings.heightOffset);
}

void BasicFogEffect::drawUI()
{
    bool changed = false;
    changed |= ImGui::DragFloat("Falloff", &settings.falloff, 0.01f, .01f, FLT_MAX);
    changed |= ImGui::DragFloat("Height offset", &settings.heightOffset, 0.05f);

    ImGui::Separator();

    if(ImGui::ColorEdit3("Absorption coefficient", &settings.absorptionCoefficient.x, ImGuiColorEditFlags_Float))
    {
        settings.absorptionCoefficient = glm::max(settings.absorptionCoefficient, .001f);
        changed = true;
    }
    changed |=
        ImGui::DragFloat("Multiplier##absorption", &settings.absorptionCoefficient.w, 0.05f, .001f, FLT_MAX);

    ImGui::Separator();
    if(ImGui::ColorEdit3("Scattering coefficient", &settings.scatteringCoefficient.x, ImGuiColorEditFlags_Float))
    {
        settings.scatteringCoefficient = glm::max(settings.scatteringCoefficient, .001f);
        changed = true;
    }
    changed |=
        ImGui::DragFloat("Multiplier##scattering", &settings.scatteringCoefficient.w, 0.05f, .001f, FLT_MAX);

    ImGui::Separator();
    changed |= ImGui::ColorEdit3(
        "Inscattered Light / Fog Color", &settings.inscatteredLight.x, ImGuiColorEditFlags_Float);
    changed |= ImGui::DragFloat("Multiplier##inscattering", &settings.inscatteredLight.w, 0.05f, .0f, FLT_MAX);

    if(changed)
    {
        updateSettings();
    }
}