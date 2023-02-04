#pragma once
#include <intern/Misc/Color.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


struct Lightbuffer
{
	// TODO: support more than directional light

	glm::vec4 direction;
	glm::vec4 color;
};

class Light
{
public:
    Light();
	~Light();

	void setDirection(const glm::vec4& direction) { m_direction = direction; }

	void setColor(const Color& color) { m_color = color; }

	void setIntensity(float intensity) { m_intensity = intensity; }

	void init();
	void bind();
	
	protected:

	glm::vec4 m_direction;

	float m_intensity;

	Color m_color;

	uint32_t m_bufferIndex;
};