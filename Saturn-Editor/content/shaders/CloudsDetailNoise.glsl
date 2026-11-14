// Volumetric Clouds (detailed Worley noise texture)
//
// This shader is based from https://github.com/qiutang98/flower/blob/dark/install/shader/cloud_detailed_noise.glsl
//

#type compute
#version 450 core

#define SAT_FREQ 8.0

#include <CloudsCommon.glsl>

// Outputs
// I don't even know what restrict does.
layout(set = 0, binding = 0, r8) restrict writeonly uniform image3D o_Output;

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

	float noise = WorleyFBM( uvw, SAT_FREQ * 1.0 ) * 0.625 + 
				  WorleyFBM( uvw, SAT_FREQ * 2.0 ) * 0.250 +
				  WorleyFBM( uvw, SAT_FREQ * 4.0 ) * 0.125;

	imageStore( o_Output, workPos, vec4( noise ) );
}
