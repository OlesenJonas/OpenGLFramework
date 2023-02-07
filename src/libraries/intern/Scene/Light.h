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

	glm::mat4 shadowMatrix;
	glm::vec4 direction;
	glm::vec4 color;
	float indirectLightIntensity;
};

class Light : public Entity
{
public:
    Light(class Scene* scene);
	~Light();

	void setDirectionVector(const glm::vec4& direction) { m_direction = direction; }

	void setDirectionFromPolarCoord(float theta, float phi)
	{
		glm::vec3 v = posFromPolar(theta, phi);
		m_lightView = glm::lookAt(200.0f * v, glm::vec3(0,0,0), glm::vec3(1.f, 0.f, 0.f));
		m_direction = glm::vec4(v, 0);
		m_shadowDirty = true;
	}

	void setColor(const Color& color) { m_color = color; }

	void setIntensity(float intensity) { m_intensity = intensity; }

	glm::vec4 direction() const { return m_direction; }

	bool castShadows() const { return m_castShadows; }
	bool shadowDirty() const { return m_shadowDirty; }

	void init(bool castShadows);
	void bind();

	void renderShadow();
	
	glm::mat4 getLightView() const { return m_lightView; }

	glm::mat4 getLightProjection() const { return m_lightProjection; }

protected:

	bool m_shadowActive;
	bool m_shadowDirty;
	glm::vec4 m_direction;
	float m_intensity;
	Color m_color;
	uint32_t m_bufferIndex;

	bool m_castShadows;
	uint32_t m_shadowMapSize;
	class Texture* m_shadowMap;
	unsigned int m_depthMapFBO;

	glm::mat4 m_lightView;
	glm::mat4 m_lightProjection;
};