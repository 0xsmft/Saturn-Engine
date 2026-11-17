//
// SkyAtmosphereSkyViewLut
//
// The following shader code is based from:
//
// Sebastien Hillaire: https://github.com/sebh/UnrealEngineSkyAtmosphere
//

#type compute
#version 450 core

#define ATMOSPHERE_MULTISCATTERING 1

#include <AtmosphereCommon.glsl>
#include <SkyViewLut.glsl>

layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D o_SkyViewLut;

layout(set = 0, binding = 8) uniform SkyViewLutData
{
	vec4 CameraPosKm;   // Planet centred
	vec4 SunDirection;  // Towards the sun
} u_SkyViewData;

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main()
{
	ivec2 size = imageSize( o_SkyViewLut );
	ivec2 px = ivec2( gl_GlobalInvocationID.xy );

	if( px.x >= size.x || px.y >= size.y )
		return;

	AtmosphereParameters atmosphere = GetAtmosphereParameters();

	vec2 uv = ( vec2( px ) + 0.5 ) / vec2( size );

	// Keep the camera inside the atmosphere and above the ground.
	float viewHeight = clamp( length( u_SkyViewData.CameraPosKm.xyz ), atmosphere.BottomRadius + 0.01, atmosphere.TopRadius - 0.01 );

	float viewZenithCosAngle;
	float lightViewCosAngle;
	UvToSkyViewLutParams( uv, viewHeight, atmosphere.BottomRadius, viewZenithCosAngle, lightViewCosAngle );

	vec3 up = normalize( u_SkyViewData.CameraPosKm.xyz + vec3( 0.0, 1e-5, 0.0 ) );
	vec3 sunWorld = normalize( u_SkyViewData.SunDirection.xyz );

	float sunZenithCosAngle = dot( up, sunWorld );
	vec3 sunDir = normalize( vec3( sqrt( max( 1.0 - sunZenithCosAngle * sunZenithCosAngle, 0.0 ) ), sunZenithCosAngle, 0.0 ) );

	float viewZenithSinAngle = sqrt( max( 1.0 - viewZenithCosAngle * viewZenithCosAngle, 0.0 ) );
	vec3 worldDir = vec3(
		viewZenithSinAngle * lightViewCosAngle,
		viewZenithCosAngle,
		viewZenithSinAngle * sqrt( max( 1.0 - lightViewCosAngle * lightViewCosAngle, 0.0 ) ) );

	vec3 worldPos = vec3( 0.0, viewHeight, 0.0 );

	SingleScatteringResult ss = IntegrateScatteredLuminance(
		vec2( px ),
		worldPos,
		worldDir,
		sunDir,
		atmosphere,
		true,
		32.0,
		-1.0,
		true,
		kDefaultMaxT,
		false );

	imageStore( o_SkyViewLut, px, vec4( ss.ScatteredLight, 1.0 ) );
}
