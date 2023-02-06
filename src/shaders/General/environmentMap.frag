#version 430

const float Pi = 3.14159265358979323846;

in vec2 passTextureCoord;

uniform layout (binding = 4) samplerCube tex;
layout(location = 3) uniform int faceIndex;
layout(location = 5) uniform float roughness;

out vec4 fragmentColor;

// Source: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint n)
{
	return vec2(float(i) / float(n), radicalInverse_VdC(i));
}

vec3 importanceSampleGGX(vec2 xi, float alpha, vec3 N)
{
	// Based on Karis 2013
	const float phi      = 2 * Pi * xi.x;
	const float cosTheta = sqrt((1 - xi.y) / (1 + (alpha * alpha - 1) * xi.y));
	const float sinTheta = sqrt(1 - cosTheta * cosTheta);

	vec3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;

	const vec3 upVector = abs(N.y) < 0.999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
	const vec3 tangentX = normalize(cross(upVector, N));
	const vec3 tangentY = cross(N, tangentX);

	return tangentX * H.x + tangentY * H.y + N * H.z;
}

const vec3 Forward[6]   = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
const vec3 Right[6]     = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};
const vec3 Up[6]        = {{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}};

void main() {

    const vec2 texCoord = vec2(2.0f, -2.0f) * passTextureCoord + vec2(-1.0f, 1.0f);
    const vec3 dir = normalize(Forward[faceIndex] + Right[faceIndex] * texCoord.x + Up[faceIndex] * texCoord.y);

    const float  alpha	= roughness * roughness;
	const vec3 N	= dir;
	const vec3 V	= N;

    float totalWeight = 0;
    vec3 preFilteredColor = vec3(0,0,0);
    const float exposure = 1.0f;

    uint SampleCount = 256;
    for(uint i = 0; i < SampleCount; ++i)
    {
        const vec2 xi = hammersley(i, SampleCount);
        const vec3 H = importanceSampleGGX(xi, alpha, N);
        const vec3 L = 2.0f * dot(V, H ) * H - V;
        const float  NdotL = clamp(dot(N, L), 0.0f, 1.0f);
        if (NdotL > 0)
        {
            const vec3 sampleColor = texture(tex, L, 0).rgb;
            preFilteredColor	+= sampleColor * exp2(exposure) * NdotL;
            totalWeight			+= NdotL;
        }
    }
	
	fragmentColor = vec4(max(preFilteredColor / totalWeight, 0), 1);
}