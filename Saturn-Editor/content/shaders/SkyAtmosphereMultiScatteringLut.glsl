//
// SkyAtmosphereMultiScatteringLut
//
// The following shader code is based from:
//
// Sebastien Hillaire: https://github.com/sebh/UnrealEngineSkyAtmosphere
//

#type compute
#version 450 core

#include <AtmosphereCommon.glsl>

layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D o_MultiScatteringLut;

// 8 x 8 directions over the sphere.
#define MS_SQRT_SAMPLE_COUNT 8

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	ivec2 size = imageSize( o_MultiScatteringLut );
	ivec2 px = ivec2( gl_GlobalInvocationID.xy );

	// Skip out of bounds.
	if( px.x >= size.x || px.y >= size.y )
		return;

	AtmosphereParameters atmosphere = GetAtmosphereParameters();

	vec2 u = vec2( px ) / vec2( max( size - 1, ivec2( 1 ) ) );

	float sunZenithCosAngle = u.x * 2.0 - 1.0;
	float viewHeight = atmosphere.BottomRadius + clamp( u.y, 0.0, 1.0 ) * ( atmosphere.TopRadius - atmosphere.BottomRadius - kAtmospherePlanetRadiusOffset ) + kAtmospherePlanetRadiusOffset;

	vec3 worldPos = vec3( 0.0, viewHeight, 0.0 );
	vec3 sunDir = vec3( sqrt( max( 1.0 - sunZenithCosAngle * sunZenithCosAngle, 0.0 ) ), sunZenithCosAngle, 0.0 );

	const float kSphereSolidAngle = 4.0 * kPI;
	const float kIsotropicPhase = 1.0 / kSphereSolidAngle;
	const float kSampleCount = float( MS_SQRT_SAMPLE_COUNT * MS_SQRT_SAMPLE_COUNT );

	vec3 multiScatAs1 = vec3( 0.0 );
	vec3 inScatteredLuminance = vec3( 0.0 );

	for( int j = 0; j < MS_SQRT_SAMPLE_COUNT; ++j )
	{
		for( int i = 0; i < MS_SQRT_SAMPLE_COUNT; ++i )
		{
			// Uniform directions over the sphere.
			float randA = ( float( i ) + 0.5 ) / float( MS_SQRT_SAMPLE_COUNT );
			float randB = ( float( j ) + 0.5 ) / float( MS_SQRT_SAMPLE_COUNT );
			float theta = 2.0 * kPI * randA;
			float phi = acos( 1.0 - 2.0 * randB );

			vec3 worldDir = vec3( cos( theta ) * sin( phi ), cos( phi ), sin( theta ) * sin( phi ) );

			SingleScatteringResult ss = IntegrateScatteredLuminance(
				vec2( px ),
				worldPos,
				worldDir,
				sunDir,
				atmosphere,
				true,
				20.0,
				-1.0,
				false,
				kDefaultMaxT,
				false );

			multiScatAs1 += ss.MultiScatAs1 * ( kSphereSolidAngle / kSampleCount );
			inScatteredLuminance += ss.ScatteredLight * ( kSphereSolidAngle / kSampleCount );
		}
	}

	multiScatAs1 *= kIsotropicPhase;
	inScatteredLuminance *= kIsotropicPhase;

	vec3 sumOfAllOrders = 1.0 / ( 1.0 - min( multiScatAs1, vec3( 0.99 ) ) );
	vec3 L = inScatteredLuminance * sumOfAllOrders * atmosphere.MultipleScatteringFactor;

	imageStore( o_MultiScatteringLut, px, vec4( L, 1.0 ) );
}
