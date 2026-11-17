//
// Volumetric Clouds - raymarching
//

#type compute
#version 450 core

#define ATMOSPHERE_MULTISCATTERING 1

#include <AtmosphereCommon.glsl>
#include <CloudsDensity.glsl>

#define CLOUDS_DEBUG_VIEW 0
#define CLOUDS_AIR_PERSPECTIVE 1
#define CLOUDS_REVERSED_Z 0
#define CLOUDS_FLIP_Y 0

// Half res trace.
layout(set = 0, binding = 9, rgba16f) uniform writeonly image2D o_Clouds;

// Distance is in km
layout(set = 0, binding = 10, r32f) uniform writeonly image2D o_CloudsDistance; 

// Distance (in km) to the weighted cloud position.
float g_CloudHitDistanceKm = 0.0;

layout(set = 0, binding = 12) uniform sampler2D u_SkyViewLut;

vec3 g_SkyAmbient = vec3( 0.0 );

#include <SkyViewLut.glsl>

vec3 ComputeSkyAmbient( vec3 cameraPosKm, vec3 sunDir, float bottomRadius )
{
	return SkyHemisphereAverage( u_SkyViewLut, cameraPosKm, sunDir, bottomRadius );
}

////////////////////////////////////////////////////////////////////////////////
// Lighting

// HGPhase from AtmosphereCommon expects the negated cosine.
float DualLobePhase( float g0, float g1, float w, float cosTheta )
{
	return mix( HGPhase( g0, -cosTheta ), HGPhase( g1, -cosTheta ), w );
}

// Optical depth (unitless) towards the sun. Samples are spaced quadratically,
// the first ones use full detail, the far ones only the base shape (HZD cone trace).
float SunOpticalDepth( vec3 posKm, vec3 sunDir, float planetRadiusKm )
{
	const float totalLen  = u_CloudsData.CloudLightBasicStep;  // km
	const int   stepCount = max( u_CloudsData.CloudLightStepNum, 1 );

	float opticalDepth = 0.0;
	float prevT = 0.0;

	for( int s = 1; s <= stepCount; ++s )
	{
		float curT = float( s ) / float( stepCount );
		curT *= curT;

		float deltaKm = ( curT - prevT ) * totalLen;
		float sampleKm = totalLen * ( prevT + curT ) * 0.5;
		prevT = curT;

		vec3 p = posKm + sunDir * sampleKm;
		CloudSample c = SampleCloud( p, planetRadiusKm, s > 3 );

		opticalDepth += c.Density * deltaKm * 1000.0;
	}

	return opticalDepth;
}

vec3 SampleAtmosphereTransmittance( vec3 posKm, vec3 sunDir, AtmosphereParameters atmosphere )
{
	float h = length( posKm );

	vec2 uv;
	LutTransmittanceParamsToUv( atmosphere, h, dot( sunDir, posKm / h ), uv );

	return textureLod( u_TransmittanceLut, uv, 0.0 ).rgb;
}

// Both roots of a ray / sphere intersection, returns false on a miss.
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

float InterleavedGradientNoise( vec2 pixel, uint frame )
{
	pixel += 5.588238 * float( frame & 63u );
	return fract( 52.9829189 * fract( 0.06711056 * pixel.x + 0.00583715 * pixel.y ) );
}

bool IsSkyDepth( float depth )
{
#if CLOUDS_REVERSED_Z
	return depth <= 0.0;
#else
	return depth >= 1.0;
#endif
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Ray march

vec4 CloudColorCompute(
	in const AtmosphereParameters atmosphere,
	vec2 uv,
	vec2 pixPos,
	float blueNoise,
	vec3 worldDir )
{
	g_CloudHitDistanceKm = u_CloudsData.CloudMaxTracingDistance;

	const float earthRadius = atmosphere.BottomRadius;
	const float radiusStart = earthRadius + CloudBottomKm();
	const float radiusEnd   = earthRadius + CloudTopKm();

	// Keep the camera slightly above the planet surface, otherwise upward rays can
	// register a ground hit at t ~ 0.
	vec3 worldPos = u_CloudsData.CameraPosKm;
	{
		float h = length( worldPos );
		const float kMinHeight = earthRadius + 0.01;
		if( h < kMinHeight )
			worldPos = ( h > 0.0 ? worldPos / h : vec3( 0.0, 1.0, 0.0 ) ) * kMinHeight;
	}

	const float viewHeight = length( worldPos );

	float tMin = 0.0;
	float tMax = 0.0;

	if( viewHeight < radiusStart )
	{
		// Below the clouds. Rays into the ground see no clouds.
		if( RayIntersectsSphereNearest( worldPos, worldDir, vec3( 0.0 ), earthRadius ) > 0.0 )
			return vec4( 0.0, 0.0, 0.0, 1.0 );

		vec2 tStart, tEnd;
		RaySphereIntersect( worldPos, worldDir, radiusStart, tStart );
		RaySphereIntersect( worldPos, worldDir, radiusEnd, tEnd );

		tMin = tStart.y;
		tMax = tEnd.y;
	}
	else if( viewHeight > radiusEnd )
	{
		// Above the clouds.
		vec2 tOuter, tInner;

		if( !RaySphereIntersect( worldPos, worldDir, radiusEnd, tOuter ) )
			return vec4( 0.0, 0.0, 0.0, 1.0 );

		tMin = tOuter.x;
		tMax = RaySphereIntersect( worldPos, worldDir, radiusStart, tInner ) ? tInner.x : tOuter.y;
	}
	else
	{
		// Inside the cloud layer.
		vec2 tInner, tOuter;
		RaySphereIntersect( worldPos, worldDir, radiusEnd, tOuter );

		if( RaySphereIntersect( worldPos, worldDir, radiusStart, tInner ) && tInner.x > 0.0 )
			tMax = tInner.x;
		else
			tMax = tOuter.y;

		tMin = 0.0;
	}

	tMin = max( tMin, 0.0 );
	tMax = max( tMax, 0.0 );

	if( tMax <= tMin || tMin > u_CloudsData.CloudTracingStartMaxDistance )
		return vec4( 0.0, 0.0, 0.0, 1.0 );

	// Fade clouds out as the entry point approaches the start distance limit, otherwise
	// the cutoff is a hard line near the horizon.
	const float horizonFade = saturate( ( u_CloudsData.CloudTracingStartMaxDistance - tMin ) / ( u_CloudsData.CloudTracingStartMaxDistance * 0.35 ) );

	const float maxDistance = u_CloudsData.CloudMaxTracingDistance;
	tMax = min( tMax, tMin + maxDistance );

	const int   kMaxIterations = 256;
	const float marchLength    = tMax - tMin;
	const float largeStep      = marchLength / float( max( u_CloudsData.CloudMarchingStepNum, 1 ) ); // km
	const float smallStep      = clamp( largeStep * 0.25, 0.03, 0.2 );                               // km

	const vec3  sunDirection = normalize( u_CloudsData.SunDirection );
	const float VoL          = dot( worldDir, sunDirection );

	// Phase for each scattering octave. Each octave is less anisotropic (Wrenninge).
	float phase[ kMsCount ];
	{
		float c = 1.0;
		for( int ms = 0; ms < kMsCount; ++ms )
		{
			phase[ ms ] = DualLobePhase(
				u_CloudsData.CloudPhaseForward * c,
				u_CloudsData.CloudPhaseBackward * c,
				u_CloudsData.CloudPhaseMixFactor,
				VoL );
			c *= 0.5;
		}
	}

	// Sun light reaching the cloud layer through the atmosphere.
	vec3 atmosphereTransmittance0 = SampleAtmosphereTransmittance( worldPos + tMin * worldDir, sunDirection, atmosphere );
	vec3 atmosphereTransmittance1 = SampleAtmosphereTransmittance( worldPos + tMax * worldDir, sunDirection, atmosphere );

	const vec3 sunIlluminance = u_CloudsData.SunColor * u_CloudsData.CloudShadingSunLightScale;

	float transmittance  = 1.0;
	vec3  scatteredLight = vec3( 0.0 );

	vec3  rayHitPos    = vec3( 0.0 );
	float rayHitWeight = 0.0;

	float t = tMin + largeStep * blueNoise;
	bool  detailed = false;
	int   emptySamples = 0;

	for( int i = 0; i < kMaxIterations; ++i )
	{
		if( t >= tMax || transmittance < 0.005 )
			break;

		vec3 samplePos = worldPos + t * worldDir;

		if( !detailed )
		{
			float coverage;
			float h = CloudHeightFraction( samplePos, earthRadius );

			if( CloudBaseShape( samplePos, h, coverage ) > 0.0 )
			{
				// Found a cloud, go back one big step and refine.
				detailed = true;
				emptySamples = 0;
				t = max( tMin, t - largeStep ) + smallStep * blueNoise;
				continue;
			}

			t += largeStep;
			continue;
		}

		CloudSample cloud = SampleCloud( samplePos, earthRadius, false );

		// Fade out at the end of the march so there's no hard cut on the horizon.
		cloud.Density *= saturate( ( tMax - t ) / ( maxDistance * 0.2 ) );

		if( cloud.Density > 0.0 )
		{
			emptySamples = 0;

			const float dtMeters = smallStep * 1000.0;
			const float sampleOpticalDepth = cloud.Density * dtMeters;
			const float stepTransmittance = exp( -sampleOpticalDepth );

			// Direct sun light, multiple scattering octaves.
			float sunOpticalDepth = SunOpticalDepth( samplePos, sunDirection, earthRadius );

			float sunLight = 0.0;
			{
				float a = 1.0;
				float b = 1.0;

				for( int ms = 0; ms < kMsCount; ++ms )
				{
					float beer = max( exp( -sunOpticalDepth * b ), exp( -sunOpticalDepth * b * 0.25 ) * 0.7 );
					sunLight += a * beer * phase[ ms ];

					a *= u_CloudsData.CloudMultiScatterScatter;
					b *= u_CloudsData.CloudMultiScatterExtinction;
				}
			}

			// Powder: dark edges on the side facing away from the sun. Uses the optical
			// depth of ~100 m around the sample, and fades out when looking towards the sun.
			float powder = 1.0 - exp( -cloud.Density * 100.0 * 2.0 * u_CloudsData.CloudPowderPow );
			powder = mix( powder, 1.0, saturate( VoL * 0.5 + 0.5 ) );
			powder = mix( 1.0, powder, u_CloudsData.CloudPowderScale );

			float marchFraction = saturate( ( t - tMin ) / max( tMax - tMin, 1e-4 ) );
			vec3 atmosphereTransmittance = mix( atmosphereTransmittance0, atmosphereTransmittance1, marchFraction );

			vec3 sunTerm = sunIlluminance * atmosphereTransmittance * sunLight * powder;

			// Ambient: sky light from above, dimmer towards the base. Ground bounce from below.
			vec3 skyAmbient = u_CloudsData.UseSkyAmbient != 0 ? g_SkyAmbient : u_CloudsData.SkyAmbientColor;

			vec3 ambientTerm =
				skyAmbient * u_CloudsData.CloudAmbientScale * mix( 0.25, 1.0, saturate( cloud.Height ) ) +
				u_CloudsData.GroundBounceColor * saturate( 1.0 - cloud.Height ) * float( u_CloudsData.CloudEnableGroundContribution != 0 );

			vec3 luminance = ( sunTerm + ambientTerm ) * u_CloudsData.CloudAlbedo;

			// Energy conserving integration over the step (Frostbite / Hillaire 2016).
			// sigmaS / sigmaE is folded into the albedo.
			scatteredLight += transmittance * luminance * ( 1.0 - stepTransmittance );

			float hitWeight = transmittance * ( 1.0 - stepTransmittance );
			rayHitPos += samplePos * hitWeight;
			rayHitWeight += hitWeight;

			transmittance *= stepTransmittance;
		}
		else if( ++emptySamples > 8 )
		{
			// Left the cloud, go back to big steps.
			detailed = false;
		}

		t += smallStep;
	}

	if( rayHitWeight > 0.0 )
		g_CloudHitDistanceKm = length( rayHitPos / rayHitWeight - worldPos );
	else
		g_CloudHitDistanceKm = max( tMin, 0.5 * ( tMin + tMax ) );

#if CLOUDS_AIR_PERSPECTIVE
	if( rayHitWeight > 0.0 )
	{
		float hitDistance = g_CloudHitDistanceKm;

		SingleScatteringResult ap = IntegrateScatteredLuminance(
			pixPos,
			worldPos,
			worldDir,
			sunDirection,
			atmosphere,
			false,        // ground
			8.0,          // sample count
			-1.0,         // no depth buffer clamp
			true,         // use Mie / Rayleigh phase
			hitDistance,
			false );

		scatteredLight = scatteredLight * ap.Transmittance + ap.ScatteredLight * u_CloudsData.SkyLightColor * ( 1.0 - transmittance );
	}
#endif

	vec4 result = vec4( scatteredLight * horizonFade, mix( 1.0, transmittance, horizonFade ) );

	if( any( isnan( result ) ) || any( isinf( result ) ) )
		result = vec4( 0.0, 0.0, 0.0, 1.0 );

	return result;
}

////////////////////////////////////////////////////////////////////////////////

layout( local_size_x = 8, local_size_y = 8 ) in;
void main()
{
	ivec2 px = ivec2( gl_GlobalInvocationID.xy );
	ivec2 size = imageSize( o_Clouds );

	if( px.x >= size.x || px.y >= size.y )
		return;

	ivec2 reconstructPx = px * 2 + ivec2( u_CloudsData.TraceOffsetX, u_CloudsData.TraceOffsetY );
	vec2 uv = ( vec2( reconstructPx ) + 0.5 ) / u_CloudsData.ReconstructSize;

	AtmosphereParameters atmosphere = GetAtmosphereParameters();

#if CLOUDS_REVERSED_Z
	const float kNearDepth = 1.0;
#else
	const float kNearDepth = 0.0;
#endif

#if CLOUDS_FLIP_Y
	vec2 ndc = vec2( uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0 );
#else
	vec2 ndc = uv * 2.0 - 1.0;
#endif

	vec4 nearClip = u_CameraData.InverseViewProj * vec4( ndc, kNearDepth, 1.0 );
	vec4 farClip  = u_CameraData.InverseViewProj * vec4( ndc, 1.0 - kNearDepth, 1.0 );
	vec3 rayDir   = normalize( farClip.xyz / farClip.w - nearClip.xyz / nearClip.w );
	vec3 worldDir = rayDir;

	float blueNoise = InterleavedGradientNoise( vec2( reconstructPx ), u_CloudsData.FrameIndex );

	if( u_CloudsData.UseSkyAmbient != 0 )
	{
		vec3 cameraPosKm = u_CloudsData.CameraPosKm;
		cameraPosKm = normalize( cameraPosKm ) * max( length( cameraPosKm ), atmosphere.BottomRadius + 0.01 );

		g_SkyAmbient = ComputeSkyAmbient( cameraPosKm, normalize( u_CloudsData.SunDirection ), atmosphere.BottomRadius ) * u_CloudsData.SkyLightColor;
	}

	vec4 clouds = CloudColorCompute( atmosphere, uv, vec2( reconstructPx ), blueNoise, worldDir );

#if CLOUDS_DEBUG_VIEW == 1
	clouds = vec4( worldDir * 0.5 + 0.5, 1.0 );
#elif CLOUDS_DEBUG_VIEW == 2
	clouds = vec4( vec3( textureLod( u_DepthTexture, uv, 0.0 ).r ), 1.0 );
#elif CLOUDS_DEBUG_VIEW == 3
	clouds = vec4( vec3( clouds.a ), 1.0 );
#elif CLOUDS_DEBUG_VIEW == 4
	clouds = vec4( vec3( IsSkyDepth( textureLod( u_DepthTexture, uv, 0.0 ).r ) ? 1.0 : 0.0 ), 1.0 );
#elif CLOUDS_DEBUG_VIEW == 5
	clouds = vec4( clouds.rgb, 1.0 );
#endif

	imageStore( o_Clouds, px, clouds );
	imageStore( o_CloudsDistance, px, vec4( g_CloudHitDistanceKm ) );
}
