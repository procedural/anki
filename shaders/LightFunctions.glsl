// Copyright (C) 2009-2015, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Contains functions for light calculations

#ifndef ANKI_SHADERS_LIGHT_FUNCTIONS_GLSL
#define ANKI_SHADERS_LIGHT_FUNCTIONS_GLSL

#pragma anki include "shaders/Common.glsl"

const float ATTENUATION_BOOST = 0.05;
const float OMNI_LIGHT_FRUSTUM_NEAR_PLANE = 0.1 / 4.0;

const uint SHADOW_SAMPLE_COUNT = 16;

//==============================================================================
/// Calculate the cluster split
uint calcClusterSplit(float zVspace)
{
	zVspace = -zVspace;
	float fk = sqrt(
		(zVspace - u_lightingUniforms.nearFarClustererDivisor.x)
		/ u_lightingUniforms.nearFarClustererDivisor.z);
	uint k = uint(fk);
	return k;
}

//==============================================================================
float computeAttenuationFactor(float lightRadius, vec3 frag2Light)
{
	float fragLightDist = dot(frag2Light, frag2Light);
	float att = 1.0 - fragLightDist * lightRadius;
	att = max(0.0, att);
	return att * att;
}

//==============================================================================
// Performs BRDF specular lighting
vec3 computeSpecularColorBrdf(
	vec3 v, // view dir
	vec3 l, // light dir
	vec3 n, // normal
	vec3 specCol,
	vec3 lightSpecCol,
	float a2, // rougness^2
	float nol) // N dot L
{
	vec3 h = normalize(l + v);

	// Fresnel (Schlick)
	float loh = max(EPSILON, dot(l, h));
	vec3 f = specCol + (1.0 - specCol) * pow((1.0 + EPSILON - loh), 5.0);
	//float f = specColor + (1.0 - specColor)
	//	* pow(2.0, (-5.55473 * loh - 6.98316) * loh);

	// NDF: GGX Trowbridge-Reitz
	float noh = max(EPSILON, dot(n, h));
	float d = a2 / (PI * pow(noh * noh * (a2 - 1.0) + 1.0, 2.0));

	// Visibility term: Geometric shadowing devided by BRDF denominator
	float nov = max(EPSILON, dot(n, v));
	float vv = nov + sqrt((nov - nov * a2) * nov + a2);
	float vl = nol + sqrt((nol - nol * a2) * nol + a2);
	float vis = 1.0 / (vv * vl);

	return f * (vis * d) * lightSpecCol;
}

//==============================================================================
vec3 computeDiffuseColor(vec3 diffCol, vec3 lightDiffCol)
{
	return diffCol * lightDiffCol;
}

//==============================================================================
float computeSpotFactor(
	vec3 l,
	float outerCos,
	float innerCos,
	vec3 spotDir)
{
	float costheta = -dot(l, spotDir);
	float spotFactor = smoothstep(outerCos, innerCos, costheta);
	return spotFactor;
}

//==============================================================================
uint computeShadowSampleCount(const uint COUNT, float zVSpace)
{
	const float MAX_DISTANCE = 5.0;

	float z = max(zVSpace, -MAX_DISTANCE);
	float sampleCountf = float(COUNT) + z * (float(COUNT) / MAX_DISTANCE);
	sampleCountf = max(sampleCountf, 1.0);
	uint sampleCount = uint(sampleCountf);

	return sampleCount;
}

//==============================================================================
float computeShadowFactorSpot(mat4 lightProjectionMat, vec3 fragPos,
	float layer, uint sampleCount)
{
	vec4 texCoords4 = lightProjectionMat * vec4(fragPos, 1.0);
	vec3 texCoords3 = texCoords4.xyz / texCoords4.w;

#if POISSON == 1
	const vec2 poissonDisk[SHADOW_SAMPLE_COUNT] = vec2[](
		vec2(0.751688, 0.619709) * 2.0 - 1.0,
		vec2(0.604741, 0.778485) * 2.0 - 1.0,
		vec2(0.936216, 0.463094) * 2.0 - 1.0,
		vec2(0.808758, 0.284966) * 2.0 - 1.0,
		vec2(0.812927, 0.786332) * 2.0 - 1.0,
		vec2(0.608651, 0.303919) * 2.0 - 1.0,
		vec2(0.482117, 0.573285) * 2.0 - 1.0,
		vec2(0.55819, 0.988451) * 2.0 - 1.0,
		vec2(0.340001, 0.728732) * 2.0 - 1.0,
		vec2(0.681775, 0.119789) * 2.0 - 1.0,
		vec2(0.217429, 0.522558) * 2.0 - 1.0,
		vec2(0.384257, 0.352163) * 2.0 - 1.0,
		vec2(0.143769, 0.738606) * 2.0 - 1.0,
		vec2(0.383474, 0.910019) * 2.0 - 1.0,
		vec2(0.409305, 0.177022) * 2.0 - 1.0,
		vec2(0.158647, 0.239097) * 2.0 - 1.0);


	float shadowFactor = 0.0;

	vec2 cordpart0 = vec2(layer, texCoords3.z);

	for(uint i = 0; i < sampleCount; i++)
	{
		vec2 cordpart1 = texCoords3.xy + poissonDisk[i] / 512.0;
		vec4 tcoord = vec4(cordpart1, cordpart0);

		shadowFactor += texture(u_spotMapArr, tcoord);
	}

	return shadowFactor / float(sampleCount);
#else
	vec4 tcoord = vec4(texCoords3.x, texCoords3.y, layer, texCoords3.z);
	float shadowFactor = texture(u_spotMapArr, tcoord);

	return shadowFactor;
#endif
}

//==============================================================================
float computeShadowFactorOmni(vec3 frag2Light, float layer, float radius)
{
	vec3 dir = (u_lightingUniforms.viewMat * vec4(-frag2Light, 1.0)).xyz;
	vec3 dirabs = abs(dir);
	float dist = -max(dirabs.x, max(dirabs.y, dirabs.z));
	dir = normalize(dir);

	const float near = OMNI_LIGHT_FRUSTUM_NEAR_PLANE;
	const float far = radius;

	// Original code:
	// float g = near - far;
	// float z = (far + near) / g * dist + (2.0 * far * near) / g;
	// float w = -dist;
	// z /= w;
	// z = z * 0.5 + 0.5;
	// Optimized:
	float z = (far * (dist + near)) / (dist * (far - near));

	float shadowFactor = texture(u_omniMapArr, vec4(dir, layer), z).r;
	return shadowFactor;
}

#endif

