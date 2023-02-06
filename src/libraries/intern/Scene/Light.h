#pragma once
#include <intern/Scene/Entity.h>
#include <intern/Misc/Color.h>
#include <glm/glm.hpp>
#include <intern/Misc/Misc.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

struct LightBuffer
{
	// TODO: support more than directional light

	glm::vec4 direction;
	glm::vec4 color;
	float indirectLightIntensity;
};

class Light : Entity
{
public:
    Light(class Scene* scene);
	~Light();

	void setDirectionVector(const glm::vec4& direction) { m_direction = direction; }

	void setDirection(float roll, float pitch, float yaw) 
	{ 
		glm::mat4 mat = glm::yawPitchRoll(glm::radians(yaw), glm::radians(pitch), glm::radians(roll));
		m_direction = glm::vec4(1,0,0,0) * mat;
	}

	void setColor(const Color& color) { m_color = color; }

	void setIntensity(float intensity) { m_intensity = intensity; }

	glm::vec4 direction() const { return m_direction; }

	void init();
	void bind();
	
protected:

	glm::vec4 m_direction;
	float m_intensity;
	Color m_color;
	uint32_t m_bufferIndex;
};