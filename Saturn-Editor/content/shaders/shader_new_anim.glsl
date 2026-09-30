// PBR Shader test
// Based on: 	PBR: A Practical Model for Physically Based Rendering (dead link)
// 				http://www.cs.utah.edu/~boulos/cs3505/papers/pbr.pdf
//				Michal Siejak, Physically Based Shading
//				https://www.siejak.pl/projects/pbr
//				Learn OpenGL
//				https://learnopengl.com
//				Yan Chernikov's (TheCherno) hazel engine
//				https://www.youtube.com/c/TheChernoProject

#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 5) in vec4 a_TransformBufferR1;
layout(location = 6) in vec4 a_TransformBufferR2;
layout(location = 7) in vec4 a_TransformBufferR3;
layout(location = 8) in vec4 a_TransformBufferR4;

layout(location = 9) in ivec4 a_BoneIndices;
layout(location = 10) in vec4 a_BoneWeights;

layout(set = 0, binding = 0) uniform Matrices 
{
	mat4 ViewProjection;
	mat4 View;
} u_Matrices;

layout(set = 0, binding = 1) uniform LightData
{
	mat4 LightMatrix[4];
};

// Set 2, owned by renderer, bone data.
layout(std430, set = 2, binding = 15) readonly buffer AnimationBoneData
{
	// 100 MAX * 1024
	mat4 Transforms[102400];
} s_AnimationBoneData;

layout(push_constant) uniform pc_MeshIndex
{
    uint Index;
} u_MeshIndex;

struct VertexOutput 
{
	vec3 Normal;
	vec3 Bionormal;
	vec3 Position;
	vec2 TexCoord;
	mat3 WorldNormals;

	mat3 CameraView;

	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout( location = 1 ) out VertexOutput vs_Output;

void main()
{
	// Index of the current mesh + instance index * MAX BONES + Indices.x/y/z
	// Explaination:
	//  - Because s_AnimationBoneData is just a buffer with transform data in order for us to get the correct data, we need to cacluate
	//    the correct offset so, imagine that this mesh was MeshIndex 1 and InstanceIndex 1
	//    we'd access the data at ( 1 + 1 ) * 100 + [index.x] = 200 + [index.x], this would place us at the correct index.
	//
	//
	mat4 skinMatrix = s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.x ] * a_BoneWeights.x;
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.y ] * a_BoneWeights.y; 
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.z ] * a_BoneWeights.z;
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.w ] * a_BoneWeights.w;

	mat4 transform = mat4( 
		a_TransformBufferR1.x, a_TransformBufferR2.x, a_TransformBufferR3.x, a_TransformBufferR4.x, 
		a_TransformBufferR1.y, a_TransformBufferR2.y, a_TransformBufferR3.y, a_TransformBufferR4.y, 
		a_TransformBufferR1.z, a_TransformBufferR2.z, a_TransformBufferR3.z, a_TransformBufferR4.z, 
		a_TransformBufferR1.w, a_TransformBufferR2.w, a_TransformBufferR3.w, a_TransformBufferR4.w  );

	vec4 localPos = skinMatrix * vec4( a_Position, 1.0 );
	vec4 WorldPos = transform * skinMatrix * vec4( a_Position, 1.0 );

	vs_Output.Position   = WorldPos.xyz;
	vs_Output.TexCoord   = vec2( a_TexCoord.x, 1.0 - a_TexCoord.y );
	vs_Output.Normal     = mat3( transform ) * mat3( skinMatrix ) * a_Normal;

	vs_Output.WorldNormals = mat3( transform ) * mat3( skinMatrix ) * mat3( a_Tangent, a_Binormal, a_Normal );

	vs_Output.Bionormal = a_Binormal;

	vs_Output.CameraView = mat3( u_Matrices.View );

	// Shadow Map Coords
	vs_Output.ShadowMapCoords[0] = LightMatrix[0] * vec4( vs_Output.Position, 1.0 );
	vs_Output.ShadowMapCoords[1] = LightMatrix[1] * vec4( vs_Output.Position, 1.0 );
	vs_Output.ShadowMapCoords[2] = LightMatrix[2] * vec4( vs_Output.Position, 1.0 );
	vs_Output.ShadowMapCoords[3] = LightMatrix[3] * vec4( vs_Output.Position, 1.0 );

	vs_Output.ViewPosition = vec3( u_Matrices.View * vec4( vs_Output.Position, 1.0 ) );

	gl_Position = u_Matrices.ViewProjection * WorldPos;
}

#type fragment
#version 450 core

#define SAT_SH_ANIMATION_PBR

#include <ShadowMappingFunctions.glsl>
#include <Lighting.glsl>
#include <ImageBasedLighting.glsl>
#include <ForwardPlus.glsl>

layout (location = 0) out vec4 FinalColor;
layout (location = 1) out vec4 OutViewNormals;
layout (location = 2) out vec4 OutAlbedo;

struct VertexOutput 
{
	vec3 Normal;
	vec3 Bionormal;
	vec3 Position;
	vec2 TexCoord;
	mat3 WorldNormals;
	
	mat3 CameraView;

	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout( location = 1 ) in VertexOutput vs_Input;

void main() 
{
	vec4 AlbedoColor = texture( u_AlbedoTexture, vs_Input.TexCoord );
	m_Params.Albedo = AlbedoColor.rgb * u_Materials.AlbedoColor;

	m_Params.Metalness = texture( u_MetallicTexture, vs_Input.TexCoord ).r * u_Materials.Metalness;
	m_Params.Roughness = texture( u_RoughnessTexture, vs_Input.TexCoord ).r * u_Materials.Roughness;
	m_Params.Roughness = max( m_Params.Roughness, 0.05 ); // Minimum roughness of 0.05 to keep specular highlight

	m_Params.Normal = normalize( vs_Input.Normal );
	if( u_Materials.UseNormalMap > 0.5 ) 
	{
		m_Params.Normal = normalize( 2.0 * texture( u_NormalTexture, vs_Input.TexCoord ).rgb - 1.0);
		m_Params.Normal = normalize( vs_Input.WorldNormals * m_Params.Normal );
	}

	OutViewNormals = vec4( vs_Input.CameraView * m_Params.Normal, 1.0 );

	m_Params.View = normalize( u_Camera.CameraPosition - vs_Input.Position );
	m_Params.NdotV = max( dot( m_Params.Normal, m_Params.View ), 0.0 );

	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

	vec3 F0 = mix( Fdielectric, m_Params.Albedo, m_Params.Metalness );

	//////////////////////////////////////////////////////////////////////////
	// SHADOWS
	uint cascadeIndex = 0;
	
	const uint SHADOW_MAP_CASCADES = 4;
	
	for( uint i = 0; i < SHADOW_MAP_CASCADES - 1; i++ )
	{
		if( vs_Input.ViewPosition.z < CascadeSplits[ i ] )
			cascadeIndex = i + 1;
	}

	vec3 ShadowCoords = (vs_Input.ShadowMapCoords[cascadeIndex].xyz / vs_Input.ShadowMapCoords[cascadeIndex].w);
	
	float ShadowAmount = HardShadows( u_ShadowMap, ShadowCoords, cascadeIndex );

	//////////////////////////////////////////////////////////////////////////
	// Output

	vec3 LightingContribution;
	vec3 iblContribution;

	LightingContribution = Lighting( F0 ) * ShadowAmount;
	iblContribution = IBL( F0, Lr );
	LightingContribution += CalculatePointLights( F0, vs_Input.Position );
	LightingContribution += m_Params.Albedo * u_Materials.Emissive;

	FinalColor = vec4( iblContribution + LightingContribution, 1.0 );

	OutAlbedo = vec4( m_Params.Albedo, 1.0 );
}
