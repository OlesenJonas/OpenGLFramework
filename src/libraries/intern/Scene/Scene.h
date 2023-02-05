#pragma once
#include <vector>

class Scene
{
public:
    Scene();
	~Scene();

    void init();
	void bind();
	void draw(const class Camera& camera);

	class Entity* createEntity();
	
	protected:

	/* Scene Lighting */

	class Light* m_sunlight;

	float m_skyRotation;
	float m_skyExposure;

    class TextureCube* m_skyTexture;
	class TextureCube* m_irradianceMap;
    class TextureCube* m_environmentMap;
    class Texture* m_BRDFIntegral;

	/* Entities */

	std::vector<class Entity*> m_entities;

	class ShaderProgram* m_defaultShader;
	class ShaderProgram* m_activeShader;
};