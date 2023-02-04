#pragma once


class Scene
{
public:
    Scene();
	~Scene();

    void init();

	void bind();
	
	protected:

	/* Scene Lighting */

	class Light* m_sunlight;

	float m_skyRotation;
	float m_skyExposure;

    class TextureCube* m_skyTexture;
	class TextureCube* m_irradianceMap;
    class TextureCube* m_environmentMap;
    class Texture* m_BRDFIntegral;
};