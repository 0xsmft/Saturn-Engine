// Screen-space AO
// This implementation of SSAO is not like a traditional one, because we don't do differed rendering and such we do not
// have a G-Buffer, we use the image from the PreDepth pass and the GeometryPass normal output as our inputs.
// This causes the final AO result to be approximation and results in less accuracy.
//
// Resources:
// AJ Weeks - https://github.com/ajweeks/FlexEngine/blob/master/FlexEngine/resources/shaders/vk_ssao.frag -- "ReconstructVSPosFromDepth"
// Sascha Willems - https://github.com/SaschaWillems/Vulkan/blob/master/shaders/glsl/ssao/ssao.frag

#type vertex
#version 450

// Inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// Outputs
layout(location = 0) out vec2 o_TexCoord;

void main() 
{	
	vec4 position = vec4( a_Position.xy, 0.0, 1.0 );
	gl_Position = position;

	o_TexCoord = a_TexCoord;
}

#type fragment
#version 450

#define SSAO_KERNEL_SIZE 32

layout(binding = 0) uniform Matrices 
{
	mat4 Proj;
	mat4 InverseProj;
} u_Matrices;

layout(binding = 1) uniform SSAO 
{
	vec4 Samples[ 32 ];
	float SSAORadius;
} u_Data;

layout( set = 0, binding = 2 ) uniform sampler2D u_DepthTexture;
layout( set = 0, binding = 3 ) uniform sampler2D u_ViewNormalTexture;
layout( set = 0, binding = 4 ) uniform sampler2D u_NoiseTexture;

layout( location = 0 ) in vec2 o_TexCoord;

vec3 ReconstructVSPosFromDepth(vec2 uv)
{
	float depth = texture(u_DepthTexture, uv).r;

	vec4 clipPos = vec4(
		uv * 2.0 - 1.0,
		depth,
		1.0
	);

	vec4 viewPos = u_Matrices.InverseProj * clipPos;
	return viewPos.xyz / viewPos.w;
}

layout( location = 0 ) out float FinalColorR;

void main() 
{
	float depth = texture( u_DepthTexture, o_TexCoord ).r;
	if( depth == 0.0f )
	{
		FinalColorR = 1.0f;
		return;
	}

	vec3 normal = normalize( texture( u_ViewNormalTexture, o_TexCoord ).rgb * 2.0f - 1.0f );
	vec3 pos = ReconstructVSPosFromDepth( o_TexCoord );

	ivec2 depthTexSize = textureSize( u_DepthTexture, 0 ); 
	ivec2 noiseTexSize = textureSize( u_NoiseTexture, 0 );

	// SSAO is rendered at 0.5x scale
	float renderScale = 0.5; 

	// Scale the noise texture so that its tiled across U and V.
	vec2 noiseUV = vec2(float(depthTexSize.x)/float(noiseTexSize.x), float(depthTexSize.y)/float(noiseTexSize.y)) * o_TexCoord * renderScale;
	vec3 randomVec = texture(u_NoiseTexture, noiseUV).xyz;
	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross( normal, tangent );
	mat3 TBN = mat3(tangent, bitangent, normal);

	float bias = 0.05;
	float occlusion = 0.0;
	int sampleCount = 0;

	for( uint i = 0; i < 32; i++ )
	{
		vec3 samplePos =
			pos + ( TBN * u_Data.Samples[ i ].xyz )
			* u_Data.SSAORadius;

		vec4 offset =
			u_Matrices.Proj * vec4( samplePos, 1.0 );

		offset.xyz /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;

		if( offset.x < 0.0 || offset.x > 1.0 ||
		   offset.y < 0.0 || offset.y > 1.0 )
		{
			continue;
		}

		vec3 reconstructedPos =
			ReconstructVSPosFromDepth( offset.xy );

		float dz =
			max( abs( pos.z - reconstructedPos.z ), 0.0001 );

		float rangeCheck =
			smoothstep( 0.0, 1.0,
			u_Data.SSAORadius / dz );

		occlusion += smoothstep(
			bias,
			u_Data.SSAORadius,
			reconstructedPos.z - samplePos.z
		) * rangeCheck;

		++sampleCount;
	}

	occlusion = 1.0 - (occlusion / float( max( sampleCount, 1 ) ) );
	FinalColorR = occlusion;
}
