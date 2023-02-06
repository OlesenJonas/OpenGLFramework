#include "Light.h"
#include <glad/glad/glad.h>
#include <intern/Scene/Scene.h>
#include <intern/Texture/Texture.h>

Light::Light(Scene* scene) : Entity(scene), m_direction(0.0f, -1.0f, 0.0f, 0.0f), m_intensity(1.0f), m_color(Color::White), m_bufferIndex(-1), m_shadowMapSize(4096)
{
}

Light::~Light()
{
}

void Light::init(bool castShadows)
{
	glGenBuffers(1, &m_bufferIndex);
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightBuffer), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	m_castShadows = castShadows;
	if (castShadows)
	{
		m_shadowMap = new Texture(TextureDesc{	.width = (GLsizei)m_shadowMapSize, 
												.height = (GLsizei)m_shadowMapSize, 
												.internalFormat = GL_DEPTH_COMPONENT32F, 
												.wrapS = GL_CLAMP_TO_EDGE,
												.wrapT = GL_CLAMP_TO_EDGE,
												.dataFormat = GL_DEPTH_COMPONENT,
												.dataType = GL_FLOAT,
												.compMode = GL_COMPARE_REF_TO_TEXTURE
												});

		glGenFramebuffers(1, &m_depthMapFBO);  
		glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap->getTextureID(), 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);  
	}
}

void Light::bind()
{
	glm::mat4 shadowMatrix = m_lightProjection * m_lightView;

	LightBuffer buffer { shadowMatrix, glm::normalize(m_direction), glm::vec4(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 1.0f), m_scene->skyExposure() };

	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightBuffer), &buffer); 

	glBindBufferBase(GL_UNIFORM_BUFFER, 21, m_bufferIndex);

	glBindTextureUnit(13, m_shadowMap->getTextureID());
}

void Light::renderShadow()
{
	m_lightProjection = glm::ortho(-250.0f, 250.0f, -250.0f, 250.0f, 10.0f, 500.0f);

	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_lightView));
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_lightProjection));
	glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
	glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
}
