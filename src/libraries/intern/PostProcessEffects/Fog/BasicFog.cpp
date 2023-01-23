#include "BasicFog.h"

#include <intern/Context/Context.h>
#include <intern/Framebuffer/Framebuffer.h>
#include <intern/PostProcessEffects/Fog/BasicFog.h>

BasicFogEffect::BasicFogEffect(int width, int height)
    : framebuffer(width, height, {{.internalFormat = GL_RGBA16F}}, false),
      shader{
          VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
          {SHADERS_PATH "/General/screenQuad.vert", SHADERS_PATH "/Fog/BasicFog.frag"}} {};

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