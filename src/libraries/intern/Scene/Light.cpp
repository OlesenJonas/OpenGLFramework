#include "Light.h"
#include <glad/glad/glad.h>

Light::Light() : m_direction(0.0f, -1.0f, 0.0f, 0.0f), m_intensity(1.0f), m_color(Color::White), m_bufferIndex(-1)
{
}

Light::~Light()
{
}

void Light::init()
{
	glGenBuffers(1, &m_bufferIndex);
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightBuffer), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Light::bind()
{

	LightBuffer buffer { glm::normalize(m_direction), glm::vec4(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 1.0f)};

	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightBuffer), &buffer); 

	glBindBufferBase(GL_UNIFORM_BUFFER, 21, m_bufferIndex);
}
