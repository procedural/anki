// Copyright (C) 2009-2022, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

ANKI_SPECIALIZATION_CONSTANT_UVEC2(FB_SIZE, 0u);
ANKI_SPECIALIZATION_CONSTANT_UVEC2(INPUT_TEX_SIZE, 2u);

#include <AnKi/Shaders/Functions.glsl>

layout(set = 0, binding = 0) uniform sampler u_linearAnyClampSampler;
layout(set = 0, binding = 1) uniform ANKI_RP texture2D u_tex;
layout(set = 0, binding = 2) uniform ANKI_RP texture2D u_lensDirtTex;

#if defined(ANKI_COMPUTE_SHADER)
const UVec2 WORKGROUP_SIZE = UVec2(16u, 16u);
layout(local_size_x = WORKGROUP_SIZE.x, local_size_y = WORKGROUP_SIZE.y, local_size_z = 1) in;

layout(set = 0, binding = 3) writeonly uniform ANKI_RP image2D u_outImg;
#else
layout(location = 0) in Vec2 in_uv;
layout(location = 0) out ANKI_RP Vec3 out_color;
#endif

// Constants
const U32 MAX_GHOSTS = 4u;
const F32 GHOST_DISPERSAL = 0.7;
const F32 HALO_WIDTH = 0.4;
const F32 CHROMATIC_DISTORTION = 3.0;
#define ENABLE_CHROMATIC_DISTORTION 1
#define ENABLE_HALO 1
const F32 HALO_OPACITY = 0.5;

ANKI_RP Vec3 textureDistorted(ANKI_RP texture2D tex, sampler sampl, Vec2 uv,
							  Vec2 direction, // direction of distortion
							  Vec3 distortion) // per-channel distortion factor
{
#if ENABLE_CHROMATIC_DISTORTION
	return Vec3(textureLod(tex, sampl, uv + direction * distortion.r, 0.0).r,
				textureLod(tex, sampl, uv + direction * distortion.g, 0.0).g,
				textureLod(tex, sampl, uv + direction * distortion.b, 0.0).b);
#else
	return textureLod(tex, uv, 0.0).rgb;
#endif
}

ANKI_RP Vec3 ssLensFlare(Vec2 uv)
{
	const Vec2 TEXEL_SIZE = 1.0 / Vec2(INPUT_TEX_SIZE);
	const Vec3 DISTORTION = Vec3(-TEXEL_SIZE.x * CHROMATIC_DISTORTION, 0.0, TEXEL_SIZE.x * CHROMATIC_DISTORTION);
	const F32 LEN_OF_HALF = length(Vec2(0.5));

	const Vec2 flipUv = Vec2(1.0) - uv;

	const Vec2 ghostVec = (Vec2(0.5) - flipUv) * GHOST_DISPERSAL;

	const Vec2 direction = normalize(ghostVec);
	ANKI_RP Vec3 result = Vec3(0.0);

	// Sample ghosts
	ANKI_UNROLL for(U32 i = 0u; i < MAX_GHOSTS; ++i)
	{
		const Vec2 offset = fract(flipUv + ghostVec * F32(i));

		ANKI_RP F32 weight = length(Vec2(0.5) - offset) / LEN_OF_HALF;
		weight = pow(1.0 - weight, 10.0);

		result += textureDistorted(u_tex, u_linearAnyClampSampler, offset, direction, DISTORTION) * weight;
	}

	// Sample halo
#if ENABLE_HALO
	const Vec2 haloVec = normalize(ghostVec) * HALO_WIDTH;
	ANKI_RP F32 weight = length(Vec2(0.5) - fract(flipUv + haloVec)) / LEN_OF_HALF;
	weight = pow(1.0 - weight, 20.0);
	result += textureDistorted(u_tex, u_linearAnyClampSampler, flipUv + haloVec, direction, DISTORTION)
			  * (weight * HALO_OPACITY);
#endif

	// Lens dirt
	result *= textureLod(u_lensDirtTex, u_linearAnyClampSampler, uv, 0.0).rgb;

	return result;
}

ANKI_RP Vec3 upscale(Vec2 uv)
{
	const ANKI_RP F32 weight = 1.0 / 5.0;
	ANKI_RP Vec3 result = textureLod(u_tex, u_linearAnyClampSampler, uv, 0.0).rgb * weight;
	result += textureLodOffset(sampler2D(u_tex, u_linearAnyClampSampler), uv, 0.0, ivec2(+1, +1)).rgb * weight;
	result += textureLodOffset(sampler2D(u_tex, u_linearAnyClampSampler), uv, 0.0, ivec2(+1, -1)).rgb * weight;
	result += textureLodOffset(sampler2D(u_tex, u_linearAnyClampSampler), uv, 0.0, ivec2(-1, -1)).rgb * weight;
	result += textureLodOffset(sampler2D(u_tex, u_linearAnyClampSampler), uv, 0.0, ivec2(-1, +1)).rgb * weight;

	return result;
}

void main()
{
#if defined(ANKI_COMPUTE_SHADER)
	if(skipOutOfBoundsInvocations(WORKGROUP_SIZE, FB_SIZE))
	{
		return;
	}

	const Vec2 uv = (Vec2(gl_GlobalInvocationID.xy) + 0.5) / Vec2(FB_SIZE);
#else
	const Vec2 uv = in_uv;
#endif

	const ANKI_RP Vec3 outColor = ssLensFlare(uv) + upscale(uv);

#if defined(ANKI_COMPUTE_SHADER)
	imageStore(u_outImg, IVec2(gl_GlobalInvocationID.xy), Vec4(outColor, 0.0));
#else
	out_color = outColor;
#endif
}
