#pragma once

#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/ShaderProgram/ShaderProgram.h>

/*
    Port of:
      https://github.com/OCASM/SSMS
    with adjustments
*/
class SSMSFogEffect
{
  public:
    SSMSFogEffect(int width, int height, uint8_t levels = 8);

    struct Settings; // Forward declare

    Settings& getSettings();
    void updateSettings();

    const Texture& execute(const Texture& colorInput, const Texture& depthInput);
    void drawUI();

    [[nodiscard]] inline const Texture& getResultColor() const
    {
        return upsampleTextures[0];
    }

    struct Settings
    {
        glm::vec4 absorptionCoefficient{0.25f, 0.25f, 0.25f, 0.0f};
        glm::vec4 scatteringCoefficient{0.25f, 0.5f, 1.0f, 0.5f};
        glm::vec4 inscatteredLight{0.9f, 0.9f, 1.0f, 1.0f};
        float falloff = 0.325f;
        float heightOffset = 0.0f;

        bool doFade = true;
        float fadeStart = 1.0f;
        float fadeLength = 1.0f;

        // SSMS parameters
        glm::vec3 blurTint{1.0f};
        float radius = 7.0f;     //[1,7]
        float blurWeight = 1.0f; //[0,100]
        float intensity = 1.0f;
    } settings;

  private:
    int width = 0;
    int height = 0;
    uint8_t levels = 0;
    float radiusAdjustedLogH = 0.0f;

    // a bit overkill with the resources allocated here, but effect is just for comparison anyways
    Texture directLight;
    Framebuffer initialFogFramebuffer;
    std::vector<Texture> downsampleTextures;
    std::vector<Texture> upsampleTextures;
    // todo: port to compute, so that switching framebuffers isnt needed?
    //  could then also use shared memory to optimize the convolutions
    std::vector<Framebuffer> downsampleFramebuffers;
    std::vector<Framebuffer> upsampleFramebuffers;

    ShaderProgram initialFogShader;
    ShaderProgram downsample0to1Shader;
    ShaderProgram downsampleShader;
    ShaderProgram upsampleShader;
    ShaderProgram upsampleAndCombineShader;

    FullscreenTri fullScreenTri;
};