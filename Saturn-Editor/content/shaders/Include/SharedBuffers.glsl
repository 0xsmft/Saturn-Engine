//
// Shared buffers for composition shaders.
//

#type header
#pragma once

// Lights

struct DirLight
{
	vec3 Direction;
	vec3 Radiance;
	float Multiplier;
};

struct PointLight
{
	vec3 Position;
	vec3 Radiance;

	float Multiplier;
	float LightSize;
	float Radius;
	float MinRadius;
	float Falloff;
};

//
// SET 0
//

layout(set = 0, binding = 2) uniform Camera 
{
	DirLight DirectionalLight;
	vec3 CameraPosition;
} u_Camera;

layout(set = 0, binding = 3) uniform ShadowData 
{
	vec4 CascadeSplits;
};

layout(set = 0, binding = 12) uniform DebugData 
{
	// Very temp.
	int TilesCountX;
} u_DebugData;

// TODO: Change number of lights...
layout(set = 0, binding = 13) uniform Lights 
{
	uint nbLights;
	PointLight Lights[32];
} u_Lights;

layout(std430, set = 0, binding = 14) buffer VisiblePointLightIndicesBuffer
{
	int Indices[];
} s_VisiblePointLightIndicesBuffer;

layout(push_constant) uniform pc_Materials
{
#if defined(SAT_SH_ANIMATION_PBR)
	layout(offset = 16) vec3 AlbedoColor;
#else
						vec3 AlbedoColor;
#endif

	float UseNormalMap;
	
	float Metalness;
	float Roughness;
	float Emissive;

} u_Materials; 

// Textures
layout (set = 0, binding = 4) uniform sampler2D u_AlbedoTexture;
layout (set = 0, binding = 5) uniform sampler2D u_NormalTexture;
layout (set = 0, binding = 6) uniform sampler2D u_MetallicTexture;
layout (set = 0, binding = 7) uniform sampler2D u_RoughnessTexture;

//
// SET 1
//
// Owned by renderer, environment settings.
//

layout (set = 1, binding = 8) uniform sampler2DArray u_ShadowMap;
layout (set = 1, binding = 9) uniform samplerCube    u_EnvRadianceTex;
layout (set = 1, binding = 10) uniform samplerCube   u_EnvIrradianceTex;
layout (set = 1, binding = 11) uniform sampler2D     u_BRDFLUTTexture;

struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
};

PBRParameters m_Params;
