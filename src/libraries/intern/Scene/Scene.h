#pragma once
#include <vector>
#include <intern/Mesh/FullscreenTri.h>

class Scene
{
public:
    Scene();
	~Scene();

    void init();
	void bind();
	void draw(const class Camera& camera);
	void updateIBL();

	class Entity* createEntity();

	class Light* sun() const { return m_sunlight; }

	float skyExposure() const { return m_skyExposure; }
	void setSkyExposure(float value) { m_skyExposure = value; }
	void setSkyboxExposure(float value) { m_skyboxExposure = value; }
	
protected:

	/* Scene Lighting */

	class Light* m_sunlight;

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

	FullscreenTri* m_tri;
	
	size_t m_irradianceMapSize;
	size_t m_environmentMapSize;
};