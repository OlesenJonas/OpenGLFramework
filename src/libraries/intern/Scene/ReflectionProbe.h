#pragma once
#include <intern/Scene/Entity.h>
#include <intern/Misc/Color.h>
#include <intern/Terrain/CBTGPU.h>
#include <glm/glm.hpp>
#include <intern/Misc/Misc.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

enum ReflectionProbeMode
{
	ReflectionProbeMode_Static,
	ReflectionProbeMode_Realtime,
};

class ReflectionProbe : public Entity
{
public:
    ReflectionProbe(class Scene* scene);
	~ReflectionProbe();
	
	void setActive(bool active) { m_active = active; m_dirty = active; }
	void setMode(ReflectionProbeMode mode) { m_mode = mode; }
	void setIntensity(float intensity) { m_intensity = intensity; }

	bool isDirty() const { return m_active && (m_dirty || m_mode == ReflectionProbeMode_Realtime); }

	class TextureCube* reflectionMap() const { return m_reflectionMap; }

	void init();
	void bind();
	void render(class CBTGPU& cbt);

protected:
	bool m_active;
	ReflectionProbeMode m_mode;
	bool m_dirty;
	float m_intensity;
	uint32_t m_bufferIndex;

	uint32_t m_textureSize;
	class TextureCube* m_reflectionMap;
	class TextureCube* m_irradianceMap;
    class TextureCube* m_environmentMap;
	uint32_t m_FBO;
    uint32_t m_RBO;
	
	friend class Scene;
};