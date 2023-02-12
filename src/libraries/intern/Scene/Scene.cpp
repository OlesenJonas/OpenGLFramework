#include "Scene.h"
#include <intern/Camera/Camera.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/Scene/Entity.h>
#include <intern/Scene/Light.h>
#include <intern/Scene/Material.h>
#include <intern/Scene/ReflectionProbe.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Texture/Texture.h>
#include <intern/Texture/TextureCube.h>

Scene::Scene()
    : m_sunlight(nullptr),
      m_reflectionProbe(nullptr),
      m_skyRotation(0.0f),
      m_skyExposure(1.0f),
      m_skyboxExposure(1.0f),
      m_skyTexture(nullptr),
      m_irradianceMap(nullptr),
      m_environmentMap(nullptr),
      m_BRDFIntegral(nullptr),
      m_activeShader(nullptr),
      m_captureFBO(-1),
      m_captureRBO(-1)
{
    m_defaultShader = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/pbs.vert", SHADERS_PATH "/General/pbs.frag"}};

    m_skyShader = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/sky.vert", SHADERS_PATH "/General/sky.frag"}};

    m_tri = new FullscreenTri();

    m_irradianceGenerationShader = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/irradianceMap.vert", SHADERS_PATH "/General/irradianceMap.frag"}};

    m_environmentMapGenerationShader = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/environmentMap.vert", SHADERS_PATH "/General/environmentMap.frag"}};

    m_shadowPass = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/shadow.vert", SHADERS_PATH "/General/shadow.frag"}};

    m_irradianceMapSize = 32;
    m_environmentMapSize = 512;
}

Scene::~Scene()
{
    for(auto& entity : m_entities)
    {
        delete entity;
    }
    m_entities.clear();

    delete m_sunlight;
    delete m_skyTexture;
    delete m_irradianceMap;
    delete m_environmentMap;
    delete m_BRDFIntegral;

    m_sunlight = nullptr;
    m_skyTexture = nullptr;
    m_irradianceMap = nullptr;
    m_environmentMap = nullptr;
    m_BRDFIntegral = nullptr;

    delete m_defaultShader;
    delete m_skyShader;
    delete m_irradianceGenerationShader;
    delete m_environmentMapGenerationShader;

    m_activeShader = nullptr;
    m_defaultShader = nullptr;
    m_skyShader = nullptr;
    m_irradianceGenerationShader = nullptr;
    m_environmentMapGenerationShader = nullptr;

    delete m_tri;
    m_tri = nullptr;
}

void Scene::init()
{
    const size_t brdfIntegralSize = 512;

    // m_skyTexture = new TextureCube(MISC_PATH "/HDRPanorama.jpg");
    // m_skyTexture = new TextureCube(MISC_PATH "/Gradient.png");
    m_skyTexture = new TextureCube(MISC_PATH "/kloppenheim_07_puresky_4k.hdr");

    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    m_irradianceMap = new TextureCube((uint32_t)m_irradianceMapSize);
    m_environmentMap = new TextureCube((uint32_t)m_environmentMapSize);
    m_BRDFIntegral = new Texture(TextureDesc{
        .width = brdfIntegralSize,
        .height = brdfIntegralSize,
        .internalFormat = GL_RGB16F,
        .wrapS = GL_CLAMP_TO_EDGE,
        .wrapT = GL_CLAMP_TO_EDGE});

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)m_irradianceMapSize, (GLsizei)m_irradianceMapSize);

    ShaderProgram BRDFIntegralGenerationShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/environmentMap.vert", SHADERS_PATH "/General/brdfIntegral.frag"}};

    glGenFramebuffers(1, &m_captureFBO);
    glGenRenderbuffers(1, &m_captureRBO);

    // BRDF Integral

    BRDFIntegralGenerationShader.useProgram();
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDFIntegral->getTextureID(), 0);
    glViewport(0, 0, brdfIntegralSize, brdfIntegralSize);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_tri->draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_sunlight = new Light(this);
    m_sunlight->init(true);
    m_sunlight->setDirectionVector(glm::vec4(0.4, 0.9f, 0.3f, 0.0f));
    m_sunlight->setColor(Color::White);
    m_sunlight->setIntensity(4.0f);

    m_reflectionProbe = new ReflectionProbe(this);
    m_reflectionProbe->init();
    m_reflectionProbe->setPosition(glm::vec3(0, 50, 0));
    updateIBL(m_skyTexture, m_irradianceMap, m_environmentMap);
}

void Scene::bind(bool reflectionProbePass)
{
    // analytical light

    if(m_sunlight)
    {
        m_sunlight->bind();
    }

    // IBL

    glBindTextureUnit(12, m_BRDFIntegral->getTextureID());
    glBindTextureUnit(13, m_skyTexture->getTextureID());

    if(!reflectionProbePass && m_reflectionProbe && m_reflectionProbe->m_active)
    {
        m_reflectionProbe->bind();
    }
    else
    {
        glBindTextureUnit(10, m_irradianceMap->getTextureID());
        glBindTextureUnit(11, m_environmentMap->getTextureID());
    }
}
static int h = 0;
void Scene::prepass(CBTGPU& cbt)
{
    // Shadows

    glCullFace(GL_FRONT);
    m_shadowPass->useProgram();
    if(m_sunlight && m_sunlight->castShadows() && m_sunlight->shadowDirty())
    {
        m_sunlight->renderShadow();

        cbt.drawDepthOnly(m_sunlight->getLightProjection() * m_sunlight->getLightView());

        m_shadowPass->useProgram();
        for(const auto& entity : m_entities)
        {
            if(!entity->m_visible)
            {
                continue;
            }
            entity->draw();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glCullFace(GL_BACK);

    // Reflection Probes

    if(m_reflectionProbe && m_reflectionProbe->isDirty())
    {
        h++;
        if(h % 30 == 0)
            m_reflectionProbe->render(cbt);
    }
}

void Scene::draw(const class Camera& camera, size_t viewportX, size_t viewportY, bool reflectionProbePass)
{
    draw(camera.getView(), camera.getProj(), camera.getSkyProj(), viewportX, viewportY, reflectionProbePass);
}

void Scene::draw(
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::mat4& skyProj,
    size_t viewportX,
    size_t viewportY,
    bool reflectionProbePass)
{
    glViewport(0, 0, (GLsizei)viewportX, (GLsizei)viewportY);
    for(const auto& entity : m_entities)
    {
        if(!entity->m_visible || reflectionProbePass && !entity->m_drawInReflectionPass)
        {
            continue;
        }

        // Bind shader
        if(entity->getMaterial()->getCustomShader())
        {
            m_activeShader = entity->getMaterial()->getCustomShader();
        }
        else
        {
            if(m_activeShader != m_defaultShader)
            {
                m_activeShader = m_defaultShader;
            }
        }
        m_activeShader->useProgram();
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));

        entity->draw();
    }

    /*		Sky		*/

    {
        m_skyShader->useProgram();
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(skyProj));
        glUniform1f(4, m_skyboxExposure);
        glDepthFunc(GL_EQUAL);
        m_tri->draw();
    }
}

void Scene::updateIBL(TextureCube* cubemap, TextureCube* irradiance, TextureCube* environment)
{
    if(!cubemap)
    {
        return;
    }

    // Irradiance Map

    m_irradianceGenerationShader->useProgram();

    glBindTextureUnit(4, cubemap->getTextureID());
    glViewport(0, 0, (GLsizei)m_irradianceMapSize, (GLsizei)m_irradianceMapSize);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    for(unsigned int i = 0; i < 6; ++i)
    {
        glUniform1i(3, i);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            irradiance->getTextureID(),
            0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_tri->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Environment Map

    m_environmentMapGenerationShader->useProgram();
    glUniform1i(3, 0);
    glBindTextureUnit(4, cubemap->getTextureID());

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    uint32_t maxMipLevels = 8;
    for(uint32_t mip = 0; mip < maxMipLevels; ++mip)
    {
        uint32_t mipSize = m_environmentMapSize * std::pow(0.5f, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        const float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(5, roughness);
        for(uint32_t i = 0; i < 6; ++i)
        {
            glUniform1i(3, i);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                environment->getTextureID(),
                mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_tri->draw();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Entity* Scene::createEntity()
{
    Entity* entity = new Entity(this);
    m_entities.emplace_back(entity);
    return entity;
}

ReflectionProbe* Scene::createReflectionProbe()
{
    ReflectionProbe* probe = new ReflectionProbe(this);
    probe->init();
    m_reflectionProbe = probe;
    m_entities.emplace_back(probe);
    return probe;
}
