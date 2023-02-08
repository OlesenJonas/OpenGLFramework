#include "ReflectionProbe.h"
#include <glad/glad/glad.h>
#include <intern/Scene/Scene.h>
#include <intern/Scene/Material.h>
#include <intern/Texture/TextureCube.h>
#include <intern/Terrain/CBTGPU.h>
#include <glm/gtx/transform.hpp>

ReflectionProbe::ReflectionProbe(Scene* scene) : 
	Entity(scene), 
	m_active(true),
	m_mode(ReflectionProbeMode_Static),
	m_intensity(1.0f), 
	m_bufferIndex(-1), 
	m_textureSize(2048), 
	m_reflectionMap(nullptr), 
	m_irradianceMap(nullptr), 
	m_environmentMap(nullptr),
	m_FBO(-1),
	m_RBO(-1)
{
	m_drawInMainPass = true;
	m_drawInReflectionPass = false;
	m_drawInShadowPass = false;
}

ReflectionProbe::~ReflectionProbe()
{
	delete m_reflectionMap;
	delete m_irradianceMap;
	delete m_environmentMap;

	m_reflectionMap = nullptr;
	m_irradianceMap = nullptr;
	m_environmentMap = nullptr;
}

void ReflectionProbe::init()
{
	m_reflectionMap = new TextureCube(m_textureSize);

    glGenFramebuffers(1, &m_FBO);
    glGenRenderbuffers(1, &m_RBO);

	m_irradianceMap = new TextureCube((uint32_t)32);
    m_environmentMap = new TextureCube((uint32_t)512);

	setMaterial(new Material(MISC_PATH "/YellowBrick_basecolor.tga", MISC_PATH "/YellowBrick_normal.tga", MISC_PATH "/YellowBrick_attributes.tga"));
	setMesh(new Mesh(MISC_PATH "/Meshes/sphere.obj"));
	setPosition(glm::vec3(0,60,0));
	getMaterial()->setCustomShader(new ShaderProgram{
		VERTEX_SHADER_BIT | FRAGMENT_SHADER_BIT,
        {SHADERS_PATH "/General/pbs.vert", SHADERS_PATH "/General/probeVisualization.frag"}});
	
}

void ReflectionProbe::bind()
{
	glBindTextureUnit(10, m_irradianceMap->getTextureID());
    glBindTextureUnit(11, m_environmentMap->getTextureID());
    //glBindTextureUnit(13, m_reflectionMap->getTextureID());
}

static const glm::mat4 CubeFaceViews[6] = {
		{  0, 0,-1, 0,		0, -1, 0, 0,		 -1, 0, 0, 0,	0, 0, 0, 1 },
		{  0, 0, 1, 0,		0, -1, 0, 0,		1, 0, 0, 0,	0, 0, 0, 1 },
		{  1, 0, 0, 0,		0, 0, -1, 0,		 0,1, 0, 0,	0, 0, 0, 1 },
		{  1, 0, 0, 0,		0, 0,1, 0,		 0, -1, 0, 0,	0, 0, 0, 1 },
		{  1, 0, 0, 0,		0, -1, 0, 0,		 0, 0, -1, 0,	0, 0, 0, 1 },
		{ -1, 0, 0, 0,		0, -1, 0, 0,		 0, 0,1, 0,	0, 0, 0, 1 }};

void ReflectionProbe::render(CBTGPU& cbt)
{
	m_dirty = false;

	m_scene->bind(true);

	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	for (int i = 0; i < 6; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_reflectionMap->getTextureID(), 0);
		glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_textureSize, m_textureSize);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		const glm::mat4 view = CubeFaceViews[i] * glm::inverse(glm::translate(glm::vec3(m_transform[3]))) * glm::rotate(0.001f, glm::vec3(0,1,0));
		const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);

		glm::mat4 viewS = view;
		viewS[3] = glm::vec4(0, 0, 0, 1);
		const glm::mat4 skyProj = glm::inverse(projection * viewS);

		m_scene->draw(view, projection, skyProj, m_textureSize, m_textureSize);
		cbt.drawDepthOnly(projection * view);

		glGenerateTextureMipmap(m_reflectionMap->getTextureID());
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_scene->updateIBL(m_reflectionMap, m_irradianceMap, m_environmentMap);
}
