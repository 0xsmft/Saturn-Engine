//
// Volumetric clouds shared resources.
//

#type header
#pragma once

#ifndef saturate
#define saturate( x ) clamp( x, 0.0, 1.0 )
#endif

layout(set = 0, binding = 0) uniform sampler2D u_WeatherMapTexture;
layout(set = 0, binding = 1) uniform sampler2D u_CurlTexture;
layout(set = 0, binding = 4) uniform sampler3D u_BasicNoiseTexture;
layout(set = 0, binding = 5) uniform sampler3D u_DetailNoiseTexture;

// Must match CloudsData in SceneRenderer.cpp (std140, size checked by a static_assert there).
layout(set = 0, binding = 8) uniform CloudsData
{
	vec3  CameraPosKm;                 // Planet centred, km. Same space as AtmosphereCommon.
	float Time;

	vec3  SunDirection;                 // Normalised, pointing TOWARDS the sun.
	float CloudSpeed;

	vec3  SunColor;                     // Sun colour * intensity (same units as your sky pass).
	float CloudCoverage;

	vec3  CloudDirection;               // Wind direction.
	float CloudDensity;

	vec3  CloudAlbedo;
	float CloudBasicNoiseScale;

	vec3  SkyAmbientColor;              // Replaces the distant lit grid. Constant sky light colour.
	float CloudDetailNoiseScale;

	vec3  GroundBounceColor;            // Replaces the cloud distant lit LUT. Light bounced up from the ground.
	float CloudWeatherUVScale;

	float CloudMaxTracingDistance;      // km
	float CloudTracingStartMaxDistance; // km
	float CloudLightBasicStep;          // km, length of the sun shadow march (try 4.0)
	float CloudPhaseForward;            // e.g. 0.6
	float CloudPhaseBackward;           // e.g. -0.3
	float CloudPhaseMixFactor;          // e.g. 0.3
	float CloudMultiScatterExtinction;  // e.g. 0.5
	float CloudMultiScatterScatter;     // e.g. 0.5
	float CloudShadingSunLightScale;    // e.g. 1.0
	float CloudAmbientScale;            // e.g. 1.0
	float CloudPowderPow;               // e.g. 1.0
	float CloudPowderScale;             // e.g. 1.0

	int   CloudMarchingStepNum;         // e.g. 64
	int   CloudLightStepNum;            // e.g. 6
	int   CloudEnableGroundContribution;
	uint  FrameIndex;

	float CloudBottomKm;                // Height of the cloud layer base above the ground.
	float CloudTopKm;                   // Height of the cloud layer top above the ground.
	float CloudType;                    // 0 = use weather map type, 1 = force cumulus.
	float CloudErosion;                 // Detail erosion strength, HZD uses 0.2.

	float CloudCurlStrength;            // km, curl distortion of the detail noise.
	int   TraceOffsetX;                 // Which pixel of each 2x2 block of the reconstruction
	int   TraceOffsetY;                 // target is traced this frame (cycles over 4 frames).
	float _Pad0;

	vec2  ReconstructSize;              // Size of the temporal reconstruction target.
	vec2  _Pad1;

	vec3  SkyLightColor;                // Sun illuminance used by the physical sky (sky-view LUT scale).
	int   UseSkyAmbient;                // 1 = ambient from the sky-view LUT, 0 = SkyAmbientColor.
} u_CloudsData;

////////////////////////////////////////////////////////////////////////////////
// References:
//   A. Schneider, "The Real-time Volumetric Cloudscapes of Horizon Zero Dawn", SIGGRAPH 2015
//   A. Schneider, "Nubis: Authoring Real-Time Volumetric Cloudscapes", SIGGRAPH 2017
//   M. Wrenninge et al., "Oz: The Great and Volumetric" (multiple scattering octaves)
//
// Units: positions in km (planet centred), extinction in 1/m.
//
// Weather map channels (content/textures/CloudsWeatherMap.tga):
//   r = coverage, g = cloud type (0 stratus, 0.5 stratocumulus, 1 cumulus)
////////////////////////////////////////////////////////////////////////////////

// Number scattering octaves.
#define kMsCount 3

// Remap with clamping.
float RemapClamped( float x, float a, float b, float c, float d )
{
	float range = b - a;
	range = abs( range ) < 1e-6 ? 1e-6 : range;

	return mix( c, d, saturate( ( x - a ) / range ) );
}

float CloudBottomKm()    { return u_CloudsData.CloudBottomKm; }
float CloudTopKm()       { return max( u_CloudsData.CloudTopKm, u_CloudsData.CloudBottomKm + 0.1 ); }
float CloudThicknessKm() { return CloudTopKm() - CloudBottomKm(); }

float CloudHeightFraction( vec3 posKm, float planetRadiusKm )
{
	return ( length( posKm ) - planetRadiusKm - CloudBottomKm() ) / CloudThicknessKm();
}

// HZD density height gradients for the cloud types.
float DensityHeightGradient( float h, float cloudType )
{
	const vec4 kStratus       = vec4( 0.00, 0.07, 0.08, 0.20 );
	const vec4 kStratocumulus = vec4( 0.00, 0.20, 0.42, 0.65 );
	const vec4 kCumulus       = vec4( 0.00, 0.12, 0.70, 1.00 );

	vec4 g = mix( kStratus, kStratocumulus, saturate( cloudType * 2.0 ) );
	g = mix( g, kCumulus, saturate( cloudType * 2.0 - 1.0 ) );

	return smoothstep( g.x, g.y, h ) * ( 1.0 - smoothstep( g.z, g.w, h ) );
}

struct CloudSample
{
	float Density;    // extinction, 1/m
	float Height;     // height fraction [0, 1]
};

vec3 CloudWindOffsetKm( float h )
{
	vec3 windDir = normalize( u_CloudsData.CloudDirection + vec3( 1e-5 ) );

	return windDir * ( u_CloudsData.Time * u_CloudsData.CloudSpeed + h * 0.5 ) + vec3( 0.0, 0.1, 0.0 ) * u_CloudsData.Time * u_CloudsData.CloudSpeed;
}

float CloudBaseShape( vec3 posKm, float h, out float coverageOut )
{
	coverageOut = 0.0;

	if( h <= 0.0 || h >= 1.0 )
		return 0.0;

	vec3 p = posKm + CloudWindOffsetKm( h );

	vec4 weather = textureLod( u_WeatherMapTexture, p.xz * u_CloudsData.CloudWeatherUVScale, 0.0 );

	float coverage = saturate( weather.r * u_CloudsData.CloudCoverage * 2.0 );
	float cloudType = saturate( mix( weather.g, 1.0, u_CloudsData.CloudType ) );

	coverageOut = coverage;

	if( coverage <= 0.001 )
		return 0.0;

	float base = textureLod( u_BasicNoiseTexture, p * u_CloudsData.CloudBasicNoiseScale, 0.0 ).r;

	// Round the bottoms and taper the tops.
	base *= DensityHeightGradient( h, cloudType );

	// Coverage: anything below (1 - coverage) is carved away, then scaled so low
	// coverage also means thinner clouds.
	float baseWithCoverage = RemapClamped( base, 1.0 - coverage, 1.0, 0.0, 1.0 );
	baseWithCoverage *= coverage;

	return baseWithCoverage;
}

CloudSample SampleCloud( vec3 posKm, float planetRadiusKm, bool cheap )
{
	CloudSample s;
	s.Height = CloudHeightFraction( posKm, planetRadiusKm );
	s.Density = 0.0;

	float coverage;
	float base = CloudBaseShape( posKm, s.Height, coverage );

	if( base <= 0.0 )
		return s;

	float shape = base;

	if( !cheap )
	{
		vec3 p = posKm + CloudWindOffsetKm( s.Height );

		// Curl noise distorts the detail noise, strongest at the bottom (turbulent bases).
		vec3 curl = textureLod( u_CurlTexture, p.xz * 0.05, 0.0 ).xyz * 2.0 - 1.0;
		p += curl * u_CloudsData.CloudCurlStrength * ( 1.0 - s.Height );

		float detail = textureLod( u_DetailNoiseTexture, p * u_CloudsData.CloudDetailNoiseScale, 0.0 ).r;

		// Wispy at the bottom, billowy at the top.
		float detailModifier = mix( detail, 1.0 - detail, saturate( s.Height * 10.0 ) );

		shape = RemapClamped( base, detailModifier * u_CloudsData.CloudErosion, 1.0, 0.0, 1.0 );
	}

	// Slightly denser towards the top, like real cumulus.
	float densityByHeight = mix( 0.6, 1.0, saturate( s.Height * 2.0 ) );

	s.Density = shape * densityByHeight * u_CloudsData.CloudDensity;
	return s;
}
