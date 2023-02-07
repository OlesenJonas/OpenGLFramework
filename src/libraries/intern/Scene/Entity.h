#pragma once
#include <glm/glm.hpp>


class Entity
{
public:
    Entity(class Scene* scene);
	~Entity();

	void draw();

	void setVisibility(bool visible) { m_visible = visible; }
	void setMaterial(class Material* material) { m_material = material; }
	void setMesh(class Mesh* mesh) { m_mesh = mesh; }

	class Material* getMaterial() const { return m_material; }

	void setPosition(const glm::vec3& position) 
	{
		m_transform[3] = glm::vec4(position.x, position.y, position.z, 1);
	}
	
protected:

	bool m_visible;
	bool m_drawInMainPass;
	bool m_drawInReflectionPass;
	bool m_drawInShadowPass;

	class Material* m_material;
	class Mesh* m_mesh;
	class Scene* m_scene;

	glm::mat4 m_transform;

	friend class Scene;
};