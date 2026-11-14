// Volumetric Clouds (basic Worley-Prelin noise texture)
//
// This shader is based from https://github.com/qiutang98/flower/blob/dark/install/shader/cloud_basic_noise.glsl
//

#type compute
#version 450 core

#include <CloudsCommon.glsl>

// Outputs
// I don't even know what restrict does.
layout(set = 0, binding = 0, r8) restrict writeonly uniform image3D o_Output;

#define SAT_BASIC_FREQ 4.0
#define SAT_BASIC_NOISE_MIX_FACTOR 0.5

float BasicNoiseComposite( vec4 v ) 
{
	float fbm = v.y * 0.625 + v.z * 0.25 + v.w * 0.125;

	return Remap( v.x, fbm - 1.0, 1.0, 0.0, 1.0 );
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() 
{
	ivec3 texSize = imageSize( o_Output );
	ivec3 workPos = ivec3( gl_GlobalInvocationID.xyz );

	// Discard if we are outside of the texture bounds.
	if( workPos.x >= texSize.x || workPos.y >= texSize.y || workPos.z >= texSize.z ) 
	{
		return;
	}

	const vec3 uvw = ( vec3( workPos ) + vec3( 0.5f ) ) / vec3( texSize );

	float fbm = mix( 1.0, PerlinFBM( uvw, SAT_BASIC_FREQ, 7 ), SAT_BASIC_NOISE_MIX_FACTOR );
	fbm = abs( fbm * 2.0 - 1.0 );

	vec4 col = vec4( 0.0 );
	col.g += WorleyFBM( uvw, SAT_BASIC_FREQ * 1.0 );
	col.b += WorleyFBM( uvw, SAT_BASIC_FREQ * 2.0 );
	col.a += WorleyFBM( uvw, SAT_BASIC_FREQ * 4.0 );
	
	col.r += Remap( fbm, 0., 1., col.g, 1.0 );

	imageStore( o_Output, workPos, vec4( BasicNoiseComposite( col ) ) );
}
