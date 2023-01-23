#pragma once

#include <intern/Framebuffer/Framebuffer.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/ShaderProgram/ShaderProgram.h>

class BasicFogEffect
{
  public:
    struct Settings
    {
    } settings;

    BasicFogEffect(int width, int height);

    const Texture& execute(const Texture& colorInput, const Texture& depthInput);

    [[nodiscard]] inline const Texture& getResultColor() const
    {
        return framebuffer.getColorTextures()[0];
    }

  private:
    Framebuffer framebuffer;
    FullscreenTri fullScreenTri;
    ShaderProgram shader;
};