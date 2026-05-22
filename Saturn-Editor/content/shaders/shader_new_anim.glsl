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

#define TRUE 1
#define FALSE 0

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

const vec3 Fdielectric = vec3( 0.04 );

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

layout(push_constant) uniform pc_Materials
{
	layout(offset = 16) vec3 AlbedoColor;
	float UseNormalMap;
	
	float Metalness;
	float Roughness;
	float Emissive;

} u_Materials; 

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

// Textures
layout (set = 0, binding = 4) uniform sampler2D u_AlbedoTexture;
layout (set = 0, binding = 5) uniform sampler2D u_NormalTexture;
layout (set = 0, binding = 6) uniform sampler2D u_MetallicTexture;
layout (set = 0, binding = 7) uniform sampler2D u_RoughnessTexture;

// Set 1, owned by renderer, environment settings.
layout (set = 1, binding = 8) uniform sampler2DArray u_ShadowMap;
layout (set = 1, binding = 9) uniform samplerCube u_EnvRadianceTex;
layout (set = 1, binding = 10) uniform samplerCube u_EnvIrradianceTex;
layout (set = 1, binding = 11) uniform sampler2D u_BRDFLUTTexture;

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

//////////////////////////////////////////////////////////////////////////
// SHADOWS
float GetShadowBias() 
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	float bias = max( MINIMUM_SHADOW_BIAS * ( 1.0 - dot( m_Params.Normal, u_Camera.DirectionalLight.Direction ) ), MINIMUM_SHADOW_BIAS );
	return bias;
}

float HardShadows( sampler2DArray ShadowMap, vec3 ShadowCoords, uint index ) 
{
	float bias = GetShadowBias();
	vec2 texelSize = 1.0 / textureSize( ShadowMap, 0 ).xy;
	vec2 invShadowMapSize = 1.0 / vec2(textureSize(ShadowMap, 0));

	float map = texture( ShadowMap, vec3( ShadowCoords.xy * 0.5 + 0.5, index ) ).x;

	// TEMP: Soft Shadows?
	float shadow = 0.0;
	float filterSize = 4.0 / 2;

	for (float x = -filterSize; x <= filterSize; x++)
    {
        for (float y = -filterSize; y <= filterSize; y++)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float text = texture(ShadowMap, vec3(ShadowCoords.xy * 0.5 + 0.5 + offset, index)).x;
            shadow += step(ShadowCoords.z, text+ bias);
        }
    }

    shadow /= ((2.0 * filterSize + 1.0) * (2.0 * filterSize + 1.0));

	return shadow;
}

//////////////////////////////////////////////////////////////////////////
// PBR
float NDFGGX(float cosLh, float r)
{
	float a = r * r;
	float alphaSq = a * a;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

float GaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

float GaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return GaSchlickG1(cosLi, k) * GaSchlickG1(NdotV, k);
}

float GeometrySchlickGGX(float NdotV, float R)
{
	float r = (R + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

//////////////////////////////////////////////////////////////////////////
// PBR-Main
vec3 Lighting( vec3 F0 ) 
{
	vec3 result = vec3( 0.0 );
	
	for( int i = 0; i < LightCount; i++ )
	{
		vec3 Li = u_Camera.DirectionalLight.Direction;
		vec3 Lradiance = u_Camera.DirectionalLight.Radiance * u_Camera.DirectionalLight.Multiplier;
		vec3 Lh = normalize( Li + m_Params.View );

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max( 0.0, dot( m_Params.Normal, Li ) );
		float cosLh = max( 0.0, dot( m_Params.Normal, Lh ) );

		vec3 F = FresnelSchlick( F0, max( 0.0, dot( Lh, m_Params.View ) ) );
		float D = NDFGGX( cosLh, m_Params.Roughness );
		float G = GaSchlickGGX( cosLi, m_Params.NdotV, m_Params.Roughness );

		vec3 kd = ( 1.0 - F ) * ( 1.0 - m_Params.Metalness );
		vec3 DiffuseBRDF = kd * m_Params.Albedo;

		vec3 SpecularBRDF = ( F * D * G ) / max( Epsilon, 4.0 * cosLi * m_Params.NdotV );
		result += ( DiffuseBRDF + SpecularBRDF ) * Lradiance * cosLi;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
// IBL
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix ={vec3(cos(angle),0.0,sin(angle)),
							vec3(0.0,1.0,0.0),
							vec3(-sin(angle),0.0,cos(angle))};
	return rotationMatrix * vec;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;

	vec3 F = FresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;
	
	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(0.0, Lr), (m_Params.Roughness) * envRadianceTexLevels).rgb;
	
	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	return kd * diffuseIBL + specularIBL;
}

//////////////////////////////////////////////////////////////////////////
// Forward+
int GetPointLightBufferIndex(int i)
{
	ivec2 tileID = ivec2(gl_FragCoord) / ivec2(16, 16);
	uint index = tileID.y * u_DebugData.TilesCountX + tileID.x;

	uint offset = index * 1024;
	return s_VisiblePointLightIndicesBuffer.Indices[offset + i];
}

int GetPointLightCount()
{
	int result = 0;
	for (int i = 0; i < u_Lights.nbLights; i++)
	{
		uint lightIndex = GetPointLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		result++;
	}

	return result;
}


//////////////////////////////////////////////////////////////////////////
// Forward+, Point Lights

vec3 CalculatePointLights(in vec3 F0, vec3 worldPos)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_Lights.nbLights; i++)
	{
		uint lightIndex = GetPointLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		PointLight light = u_Lights.Lights[lightIndex];
		vec3 Li = normalize(light.Position - worldPos);
		float lightDistance = length(light.Position - worldPos);
		vec3 Lh = normalize(Li + m_Params.View);

		float attenuation = clamp(1.0 - lightDistance * lightDistance / (light.Radius * light.Radius * 10), 0.0, 1.0);
		attenuation *= mix(attenuation, 1.0, light.Falloff);

		vec3 Lradiance = light.Radiance * light.Multiplier * attenuation;

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
		float D = NDFGGX(cosLh, m_Params.Roughness);
		float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

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
