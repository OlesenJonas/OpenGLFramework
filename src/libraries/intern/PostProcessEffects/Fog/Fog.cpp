#include "Fog.h"
#include "PostProcessEffects/Fog/BasicFog.h"
#include "PostProcessEffects/Fog/SSMSFog.h"

#include <ImGui/imgui.h>

FogEffect::FogEffect(int width, int height) : basicFogEffect(width, height), ssmsFogEffect(width, height, 8)
{
    basicFogEffect.updateSettings();
    ssmsFogEffect.updateSettings();
};

const Texture& FogEffect::execute(const Texture& colorInput, const Texture& depthInput)
{
    if(mode == 0)
    {
        return basicFogEffect.execute(colorInput, depthInput);
    }
    if(mode == 1)
    {
        return ssmsFogEffect.execute(colorInput, depthInput);
    }
    assert(false);
}

void FogEffect::drawUI()
{
    int oldMode = mode;
    if(ImGui::Combo("Rendering Mode", &mode, "Basic\0SSMS\0"))
    {
        if(mode != oldMode)
            changeRenderingMode();
    }
    ImGui::Separator();
    if(mode == 0)
    {
        basicFogEffect.drawUI();
    }
    else if(mode == 1)
    {
        ssmsFogEffect.drawUI();
    }
}

void FogEffect::changeRenderingMode()
{
    if(mode == 0)
    {
        auto& oldSettings = ssmsFogEffect.getSettings();
        auto& newSettings = basicFogEffect.getSettings();
        newSettings.absorptionCoefficient = oldSettings.absorptionCoefficient;
        newSettings.scatteringCoefficient = oldSettings.scatteringCoefficient;
        newSettings.inscatteredLight = oldSettings.inscatteredLight;
        newSettings.falloff = oldSettings.falloff;
        newSettings.heightOffset = oldSettings.heightOffset;
        basicFogEffect.updateSettings();
        return;
    }
    if(mode == 1)
    {
        auto& oldSettings = basicFogEffect.getSettings();
        auto& newSettings = ssmsFogEffect.getSettings();
        newSettings.absorptionCoefficient = oldSettings.absorptionCoefficient;
        newSettings.scatteringCoefficient = oldSettings.scatteringCoefficient;
        newSettings.inscatteredLight = oldSettings.inscatteredLight;
        newSettings.falloff = oldSettings.falloff;
        newSettings.heightOffset = oldSettings.heightOffset;
        ssmsFogEffect.updateSettings();
        return;
    }
}