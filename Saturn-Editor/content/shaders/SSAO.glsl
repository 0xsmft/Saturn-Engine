// Based from https://github.com/ajweeks/FlexEngine/blob/master/FlexEngine/resources/shaders/vk_ssao.frag for "reconstructVSPosFromDepth"
// Also https://github.com/SaschaWillems/Vulkan/tree/master/ for the bulk of the code

#type vertex
#version 450

// Inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// Outputs
layout(location = 0) out vec2 o_TexCoord;

void main() 
{	
	vec4 position = vec4( a_Position.xy, 1.0, 1.0 );
	gl_Position = position;

	o_TexCoord = a_TexCoord;
}

#type fragment
#version 450

#define SSAO_KERNEL_SIZE = 32;

layout(binding = 0) uniform Matrices 
{
	mat4 VP;
	mat4 InverseVP;
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

vec3 reconstructVSPosFromDepth(vec2 uv)
{
  float depth = texture(u_DepthTexture, uv).r;

  float x = uv.x * 2.0 - 1.0;
  float y = (1.0 - uv.y) * 2.0 - 1.0;
  vec4 pos = vec4(x, y, depth, 1.0);
  vec4 posVS = u_Matrices.InverseVP * pos;

  return posVS.xyz / posVS.w;
}

layout( location = 0 ) out vec4 FinalColor;

void main() 
{
	float depth = texture( u_DepthTexture, o_TexCoord ).r;

	vec3 normal = normalize( texture( u_ViewNormalTexture, o_TexCoord ).rgb * 2.0f - 1.0f );
	vec3 pos = reconstructVSPosFromDepth( o_TexCoord );

	ivec2 depthTexSize = textureSize( u_DepthTexture, 0 ); 
	ivec2 noiseTexSize = textureSize( u_NoiseTexture, 0 );

	// SSAO is rendered at 0.5x scale
	float renderScale = 0.5; 
	vec2 noiseUV = vec2(float(depthTexSize.x)/float(noiseTexSize.x), float(depthTexSize.y)/float(noiseTexSize.y)) * o_TexCoord * renderScale;
	vec3 randomVec = texture(u_NoiseTexture, noiseUV).xyz;
	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross( normal, tangent  );
	mat3 TBN = mat3(tangent, bitangent, normal);

	float bias = 0.01f;

	float occlusion = 0.0f;
	int sampleCount = 0;
	for (uint i = 0; i < 32; i++)
	{
		vec3 samplePos = pos + ( TBN * u_Data.Samples[ i ].xyz ) * u_Data.SSAORadius;

		vec4 offset = u_Matrices.VP * vec4(samplePos, 1.0f);
		offset.xyz /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;
		offset.y = 1.0f - offset.y;
		
		if( offset.x < 0.0f || offset.y > 1.0 || offset.y < 0.0f || offset.x > 1.0f )
			continue;

		vec3 reconstructedPos = reconstructVSPosFromDepth(offset.xy);

		float rangeCheck = smoothstep(0.0f, 1.0f, u_Data.SSAORadius / abs(reconstructedPos.z - samplePos.z));

		occlusion += (reconstructedPos.z >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / 32);
	FinalColor = vec4(occlusion);
}
