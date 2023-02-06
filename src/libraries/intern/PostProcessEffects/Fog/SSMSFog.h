#pragma once

#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/ShaderProgram/ShaderProgram.h>

/*
    Port of:
      https://github.com/OCASM/SSMS
    with adjustments
      Driving fog with exponential height fog
      Using just a single mip chain
      Splitting radiance into scattered and non scattered amount as in
        https://elek.pub/projects/CGA2013/Elek2013.pdf
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
        return textures[0];
    }

    struct Settings
    {
        glm::vec4 absorptionCoefficient{0.25f, 0.25f, 0.25f, 0.1f};
        glm::vec4 scatteringCoefficient{0.25f, 0.5f, 1.0f, 0.5f};
        glm::vec4 inscatteredLight{0.9f, 0.9f, 1.0f, 1.0f};
        float falloff = 0.325f;
        float heightOffset = 0.0f;

        // SSMS parameters
        glm::vec3 blurTint{1.0f};
        float steps = 2.0f;
        float blurWeight = 1.0f;
        float intensity = 1.0f;
        bool approxInscatterTransmittance = false;
    } settings;

  private:
    int width = 0;
    int height = 0;
    uint8_t levels = 0;

    // a bit overkill with the resources allocated here, but effect is just for comparison anyways
    Texture directLight;
    Texture blurWeight;
    Framebuffer initialFogFramebuffer;
    std::vector<Texture> textures;
    // todo: port to compute, so that switching framebuffers isnt needed?
    //  could then also use shared memory to optimize the convolutions
    std::vector<Framebuffer> framebuffers;

    ShaderProgram initialFogShader;
    ShaderProgram downsample0to1Shader;
    ShaderProgram downsampleShader;
    ShaderProgram upsampleShader;
    ShaderProgram upsampleAndCombineShader;

    FullscreenTri fullScreenTri;
};