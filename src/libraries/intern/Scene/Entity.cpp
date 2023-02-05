#include "Entity.h"
#include <glad/glad/glad.h>
#include <intern/Scene/Material.h>
#include <intern/Scene/Scene.h>
#include <intern/Mesh/Mesh.h>

Entity::Entity(Scene* scene) : m_material(nullptr), m_mesh(nullptr), m_scene(scene)
{
	m_transform = glm::mat4(1);
}

Entity::~Entity()
{
	delete m_material;
	delete m_mesh;

	m_material = nullptr;
	m_mesh = nullptr;
}

void Entity::draw()
{
	if (m_material && m_mesh)
	{
		m_material->bind(MaterialChannel_1);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_transform));
		m_mesh->draw();
	}
}
