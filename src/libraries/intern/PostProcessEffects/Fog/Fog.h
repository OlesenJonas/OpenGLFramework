#pragma once

#include "BasicFog.h"
#include "SSMSFog.h"

class FogEffect
{
  public:
    struct Settings;

    FogEffect(int width, int height);

    const Texture& execute(const Texture& colorInput, const Texture& depthInput);
    void drawUI();

  private:
    void changeRenderingMode();
    int mode = 0;
    BasicFogEffect basicFogEffect;
    SSMSFogEffect ssmsFogEffect;
};