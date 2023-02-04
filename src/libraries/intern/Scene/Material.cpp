#include <stdint.h>
#include "Material.h"

Material::Material() : m_baseMap(nullptr), m_normalMap(nullptr), m_attributesMap(nullptr)
{
}

Material::~Material()
{
	delete m_baseMap;
	delete m_normalMap;
	delete m_attributesMap;

	m_baseMap = nullptr;
	m_normalMap = nullptr;
	m_attributesMap = nullptr;
}

void Material::bind(uint32_t materialChannel)
{
}
