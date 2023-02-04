#pragma once
#include <stdint.h>
#include <intern/Misc/Color.h>

class Material
{
public:

	Material();
	~Material();

	void setBaseColor(const Color& color) { m_baseColor = color; }

	void bind(uint32_t materialChannel = 0);

protected:

	Color m_baseColor;

	float m_normalIntensity;

	class Texture* m_baseMap;

	class Texture* m_normalMap;

	class Texture* m_attributesMap;
};