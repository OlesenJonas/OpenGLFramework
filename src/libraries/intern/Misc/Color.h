#pragma once

struct Color
{
    float r, g, b, a;

    Color() : r(0), g(0), b(0), a(1)
    {}

	Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a)
    {}

	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
};
