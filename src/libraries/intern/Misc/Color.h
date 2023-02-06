#pragma once
#include <glm/glm.hpp>

struct Color
{
    float r, g, b, a;

    Color() : r(0), g(0), b(0), a(1)
    {}

	Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a)
    {}

	operator glm::vec4() const
	{
		return glm::vec4(r, g, b, a);
	}

	operator glm::vec3() const
	{
		return glm::vec3(r, g, b);
	}

	// Based on https://github.com/chendi-YU/UnrealEngine/blob/master/Engine/Shaders/PostProcessCombineLUTs.usf
	static Color fromTemperature(float temperature)
	{
		temperature = std::min(std::max(temperature, 1000.0f), 15000.0f);

		// Approximate Planckian locus in CIE 1960 UCS
		float u = (0.860117757f + 1.54118254e-4f * temperature + 1.28641212e-7f * temperature*temperature) / (1.0f + 8.42420235e-4f * temperature + 7.08145163e-7f * temperature*temperature);
		float v = (0.317398726f + 4.22806245e-5f * temperature + 4.20481691e-8f * temperature*temperature) / (1.0f - 2.89741816e-5f * temperature + 1.61456053e-7f * temperature*temperature);

		float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
		float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
		float z = 1.0f - x - y;

		float Y = 1.0f;
		float X = Y / y * x;
		float Z = Y / y * z;

		// XYZ to RGB with BT.709 primaries
		float r = 3.2404542f * X + -1.5371385f * Y + -0.4985314f * Z;
		float g = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
		float b = 0.0556434f * X + -0.2040259f * Y + 1.0572252f * Z;

		return Color(r, g, b, 1.0f);
	}

	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
};
