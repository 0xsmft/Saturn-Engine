//
// Volumetric Clouds shadow mapping
//

#type compute
#version 450 core

#include <CloudsDensity.glsl>

// Matching CLOUD_SHADOW_EXTENT_M in Lighting.glsl.
#define CLOUD_SHADOW_EXTENT_M 40000.0

layout(set = 0, binding = 9, r32f) uniform writeonly image2D o_CloudShadow;

layout(set = 0, binding = 11) uniform CloudShadowData
{
	vec4  CameraPosMeters;
	float PlanetRadiusKm;
	float ShadowStrength;
	int   StepCount;
	float MinLight;
} u_ShadowData;

bool RaySphereIntersect( vec3 ro, vec3 rd, float r, out vec2 t )
{
	float b = dot( ro, rd );
	float c = dot( ro, ro ) - r * r;
	float h = b * b - c;

	if( h < 0.0 )
	{
		t = vec2( -1.0 );
		return false;
	}

	h = sqrt( h );
	t = vec2( -b - h, -b + h );

	return true;
}

layout( local_size_x = 8, local_size_y = 8 ) in;
void main()
{
	ivec2 px = ivec2( gl_GlobalInvocationID.xy );
	ivec2 size = imageSize( o_CloudShadow );

	if( px.x >= size.x || px.y >= size.y )
		return;

	vec3 sunDir = normalize( u_CloudsData.SunDirection );

	if( sunDir.y <= 0.02 )
	{
		imageStore( o_CloudShadow, px, vec4( 1.0 ) );
		return;
	}

	vec2 uv = ( vec2( px ) + 0.5 ) / vec2( size );
	vec2 groundXZ = u_ShadowData.CameraPosMeters.xz + ( uv - 0.5 ) * CLOUD_SHADOW_EXTENT_M;

	const float planetRadius = u_ShadowData.PlanetRadiusKm;
	vec3 groundKm = vec3( groundXZ.x * 0.001, planetRadius + 0.001, groundXZ.y * 0.001 );

	// The ground point is below the layer, so the far roots are the entry and exit.
	vec2 tBottom, tTop;
	RaySphereIntersect( groundKm, sunDir, planetRadius + CloudBottomKm(), tBottom );
	RaySphereIntersect( groundKm, sunDir, planetRadius + CloudTopKm(), tTop );

	float tStart = max( tBottom.y, 0.0 );
	float tEnd   = max( tTop.y, tStart );

	const int stepCount = max( u_ShadowData.StepCount, 1 );
	const float dtKm = ( tEnd - tStart ) / float( stepCount );

	float opticalDepth = 0.0;

	for( int i = 0; i < stepCount; ++i )
	{
		// Fixed mid-step offset, a per-frame jitter would make the shadows flicker.
		vec3 p = groundKm + sunDir * ( tStart + ( float( i ) + 0.5 ) * dtKm );

		CloudSample c = SampleCloud( p, planetRadius, false );
		opticalDepth += c.Density * dtKm * 1000.0;
	}

	float transmittance = exp( -opticalDepth * u_ShadowData.ShadowStrength );
	transmittance = mix( u_ShadowData.MinLight, 1.0, transmittance );

	imageStore( o_CloudShadow, px, vec4( transmittance ) );
}
