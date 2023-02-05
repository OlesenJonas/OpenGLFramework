#pragma once
#include <stdint.h>
#include <intern/Misc/Color.h>
#include <glm/glm.hpp>
#include <string>

enum MaterialChannel
{
	MaterialChannel_1 = 0,
	MaterialChannel_2 = 1,
	MaterialChannel_3 = 2,
	MaterialChannel_4 = 3,
};

struct MaterialBuffer
{
	glm::vec4 basecolor;
	float normalIntensity;
};


class Material
{
public:

	Material(class Texture* baseMap = nullptr, class Texture* normalMap = nullptr, class Texture* attributesMap = nullptr);
	Material(const std::string& baseMap, const std::string& normalMap, const std::string& attributesMap);

	~Material();

	void setBaseColor(const Color& color) { m_baseColor = color; }

	void bind(MaterialChannel materialChannel = MaterialChannel_1);

	class ShaderProgram* getCustomShader() const { return m_shader; }

protected:

	Color m_baseColor;
	float m_normalIntensity;
	class Texture* m_baseMap;
	class Texture* m_normalMap;
	class Texture* m_attributesMap;

	uint32_t m_bufferIndex;

	class ShaderProgram* m_shader;
};