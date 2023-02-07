#pragma once
#include <vector>
#include <intern/Mesh/FullscreenTri.h>

class Scene
{
public:
    Scene();
	~Scene();

    void init();
	void bind(bool reflectionProbePass = false);
	void prepass(class CBTGPU& cbt);
	void draw(const class Camera& camera, size_t viewportX, size_t viewportY, bool reflectionProbePass = false);
	void draw(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& skyProj, size_t viewportX, size_t viewportY, bool reflectionProbePass = false);
	void updateIBL(class TextureCube* cubemap, TextureCube* irradiance, TextureCube* environment);

	class Entity* createEntity();

	class ReflectionProbe* createReflectionProbe();

	class Light* sun() const { return m_sunlight; }
	class ReflectionProbe* reflectionProbe() const { return m_reflectionProbe; }

	float skyExposure() const { return m_skyExposure; }
	void setSkyExposure(float value) { m_skyExposure = value; }
	void setSkyboxExposure(float value) { m_skyboxExposure = value; }
	
protected:

	/* Scene Lighting */

	class Light* m_sunlight;

	class ReflectionProbe* m_reflectionProbe;

	float m_skyRotation;
	float m_skyExposure;
	float m_skyboxExposure;

    class TextureCube* m_skyTexture;
	class TextureCube* m_irradianceMap;
    class TextureCube* m_environmentMap;
    class Texture* m_BRDFIntegral;

	/* Entities */

	std::vector<class Entity*> m_entities;

	class ShaderProgram* m_skyShader;
	class ShaderProgram* m_defaultShader;
	class ShaderProgram* m_activeShader;

	class ShaderProgram* m_irradianceGenerationShader;
	class ShaderProgram* m_environmentMapGenerationShader;
	class ShaderProgram* m_shadowPass;

	FullscreenTri* m_tri;
	
	size_t m_irradianceMapSize;
	size_t m_environmentMapSize;

	unsigned int m_captureFBO;
    unsigned int m_captureRBO;
};