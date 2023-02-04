#version 430

const float Pi = 3.14159265358979323846;

in vec2 passTextureCoord;
out vec4 fragmentColor;

float specularG1(float NdotV, float k)
{
	return NdotV / (NdotV * (1 - k) + k);
}

float specularG(float NdotL, float NdotV, float LdotH, float alpha)
{
	const float k = alpha / 2;
	
	// Based on Karis 2013
	const float g1 = specularG1(NdotL, k);
	const float g2 = specularG1(NdotV, k);
	return g1 * g2;
}


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

void main() {
    const uint BRDFSampleCount = 512;
    float roughness = passTextureCoord.x;
    float NdotV = passTextureCoord.y;
 

    // Based on Karis 2013
	const float alpha = roughness * roughness;
	const vec3 V = vec3(sqrt(1 - NdotV * NdotV), 0, NdotV);
	const vec3 N = vec3(0, 0, 1);

	float A = 0;
	float B = 0;
	for (uint i = 0; i < BRDFSampleCount; ++i)
	{
		const vec2 xi = hammersley(i, BRDFSampleCount);
		const vec3 H = importanceSampleGGX(xi, alpha, N);
		const vec3 L = 2 * dot(V, H) * H - V;

		const float NdotL = clamp(L.z,0.0,1.0);
		const float NdotH = clamp(H.z,0.0,1.0);
		const float LdotH = clamp(dot(L, H),0.0,1.0);
		const float VdotH = clamp(dot(V, H),0.0,1.0);

		if (NdotL > 0)
		{
			const float G		= specularG(NdotL, NdotV, LdotH, alpha);
			const float G_Vis	= G * VdotH / (NdotH * NdotV);
			const float Fc		= pow(1 - VdotH, 5);

			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	fragmentColor = vec4(A / BRDFSampleCount, B / BRDFSampleCount, 0, 1);
}