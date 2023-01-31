#pragma once

#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/ShaderProgram/ShaderProgram.h>

class BasicFogEffect
{

  public:
    struct Settings;

    BasicFogEffect(int width, int height);

    Settings& getSettings();
    void updateSettings();

    const Texture& execute(const Texture& colorInput, const Texture& depthInput);
    void drawUI();

    [[nodiscard]] inline const Texture& getResultColor() const
    {
        return framebuffer.getColorTextures()[0];
    }

    struct Settings
    {
        glm::vec4 absorptionCoefficient{0.25f, 0.25f, 0.25f, 1.0f};
        glm::vec4 scatteringCoefficient{0.25f, 0.25f, 0.25f, 1.0f};
        glm::vec4 extinctionCoefficient{0.25f, 0.25f, 0.25f, 0.15f};
        glm::vec4 inscatteredLight{0.9f, 0.9f, 1.0f, 1.0f};
        float falloff = 0.325f;
        float heightOffset = 0.0f;
        int mode = 0;
    } settings;

  private:
    Framebuffer framebuffer;
    FullscreenTri fullScreenTri;
    ShaderProgram shader;
};