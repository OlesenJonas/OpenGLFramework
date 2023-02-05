#include "Scene.h"
#include <intern/Texture/TextureCube.h>
#include <intern/Texture/Texture.h>
#include <intern/ShaderProgram/ShaderProgram.h>
#include <intern/Mesh/FullscreenTri.h>
#include <intern/Scene/Light.h>
#include <intern/Scene/Entity.h>
#include <intern/Scene/Material.h>
#include <intern/Camera/Camera.h>

Scene::Scene() : m_sunlight(nullptr), m_skyRotation(0.0f), m_skyExposure(1.0f), m_skyTexture(nullptr), m_irradianceMap(nullptr), m_environmentMap(nullptr), m_BRDFIntegral(nullptr), m_activeShader(nullptr)
{
	m_defaultShader = new ShaderProgram{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/pbs.vert", SHADERS_PATH "/General/pbs.frag"}};
}

Scene::~Scene()
{
	for (auto& entity : m_entities)
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
}

void Scene::init()
{
	const size_t irradianceMapSize = 32;
    const size_t environmentMapSize = 512;
    const size_t brdfIntegralSize = 512;

    m_skyTexture = new TextureCube(MISC_PATH "/HDRPanorama.jpg");
    FullscreenTri tri = FullscreenTri();

    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    m_irradianceMap = new TextureCube(irradianceMapSize);
    m_environmentMap = new TextureCube(environmentMapSize);
    m_BRDFIntegral = new Texture(TextureDesc{.width = brdfIntegralSize, .height = brdfIntegralSize, .internalFormat = GL_RGB16F});

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);

    ShaderProgram irradianceGenerationShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/irradianceMap.vert", SHADERS_PATH "/General/irradianceMap.frag"}};

    ShaderProgram EnvironmentMapGenerationShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/environmentMap.vert", SHADERS_PATH "/General/environmentMap.frag"}};

    ShaderProgram BRDFIntegralGenerationShader{
        VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/environmentMap.vert", SHADERS_PATH "/General/brdfIntegral.frag"}};


    // BRDF Integral

    BRDFIntegralGenerationShader.useProgram();
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDFIntegral->getTextureID(), 0);
    glViewport(0, 0, brdfIntegralSize, brdfIntegralSize);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    tri.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    // Irradiance Map

    irradianceGenerationShader.useProgram();
    glBindTextureUnit(4, m_skyTexture->getTextureID());
    glViewport(0, 0, irradianceMapSize, irradianceMapSize);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for(unsigned int i = 0; i < 6; ++i)
    {
        glUniform1i(3, i);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            m_irradianceMap->getTextureID(),
            0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        tri.draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Environment Map

    EnvironmentMapGenerationShader.useProgram();
    glUniform1i(3, 0);
    glBindTextureUnit(4, m_skyTexture->getTextureID());

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    uint32_t maxMipLevels = 8;
    for(uint32_t mip = 0; mip < maxMipLevels; ++mip)
    {
        uint32_t mipSize = environmentMapSize * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        const float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(5, roughness);
        for(uint32_t i = 0; i < 6; ++i)
        {
            glUniform1i(3, i);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_environmentMap->getTextureID(), mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            tri.draw();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_sunlight = new Light();
	m_sunlight->init();
	m_sunlight->setDirection(glm::vec4(0.4, 0.9f, 0.3f, 0.0f));
	m_sunlight->setColor(Color::White);
	m_sunlight->setIntensity(2.0f);
}

void Scene::bind()
{
	// analytical light

	if (m_sunlight)
	{
		m_sunlight->bind();
	}

	// IBL

    glBindTextureUnit(10, m_irradianceMap->getTextureID());
    glBindTextureUnit(11, m_environmentMap->getTextureID());
    glBindTextureUnit(12, m_BRDFIntegral->getTextureID());
    glBindTextureUnit(13, m_skyTexture->getTextureID());
}

void Scene::draw(const class Camera& camera)
{
	for (const auto& entity : m_entities)
	{
		// Bind shader
		if (entity->getMaterial()->getCustomShader())
		{
			m_activeShader = entity->getMaterial()->getCustomShader();
		}
		else
		{
			if (m_activeShader != m_defaultShader)
			{
				m_activeShader = m_defaultShader;
			}
		}
		m_activeShader->useProgram();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(*camera.getView()));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*camera.getProj()));

		entity->draw();
	}
}

Entity* Scene::createEntity()
{
	Entity* entity = new Entity(this);
	m_entities.emplace_back(entity);
	return entity;
}
