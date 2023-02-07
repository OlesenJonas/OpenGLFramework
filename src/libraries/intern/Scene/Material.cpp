#include <stdint.h>
#include "Material.h"
#include <intern/Texture/Texture.h>
#include <glad/glad/glad.h>

Material::Material(Texture* baseMap, Texture* normalMap, Texture* attributesMap) : 
	m_baseColor(Color::White), 
	m_normalIntensity(1.0f), 
	m_roughness(1.0f), 
	m_metallic(1.0f), 
	m_occlusion(1.0f), 
	m_baseMap(baseMap), 
	m_normalMap(normalMap), 
	m_attributesMap(attributesMap), 
	m_bufferIndex(0), 
	m_shader(nullptr) 
	{
		glGenBuffers(1, &m_bufferIndex);
		glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialBuffer), NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

Material::Material(const std::string& baseMap, const std::string& normalMap, const std::string& attributesMap) : 
	Material(new Texture(baseMap, false, true), new Texture(normalMap, true, true), new Texture(attributesMap, false, true)) { }


Material::~Material()
{
	delete m_baseMap;
	delete m_normalMap;
	delete m_attributesMap;

	m_baseMap = nullptr;
	m_normalMap = nullptr;
	m_attributesMap = nullptr;
}

void Material::bind(MaterialChannel materialChannel)
{
	// Update buffer
	MaterialBuffer buffer { glm::vec4(m_baseColor), glm::vec4(m_roughness, m_metallic, m_occlusion, m_normalIntensity) };

	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferIndex);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MaterialBuffer), &buffer); 

	// Bind buffer
	const int BufferIndexOffset = (int)materialChannel;
	glBindBufferBase(GL_UNIFORM_BUFFER, 22 + BufferIndexOffset, m_bufferIndex);

	// Bind textures
	const int TextureIndexOffset = (int)materialChannel * 3;
	if (m_baseMap)
		glBindTextureUnit(1 + TextureIndexOffset, m_baseMap->getTextureID());
	if (m_normalMap)
		glBindTextureUnit(2 + TextureIndexOffset, m_normalMap->getTextureID());
	if (m_attributesMap)
		glBindTextureUnit(3 + TextureIndexOffset, m_attributesMap->getTextureID());
}
