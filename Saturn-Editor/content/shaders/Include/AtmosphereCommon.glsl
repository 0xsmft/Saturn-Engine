//
// Shared functions for the atmosphere.
//

#type header
#pragma once

layout (set = 0, binding = 2) uniform sampler2D u_TransmittanceLut;
layout (set = 0, binding = 3) uniform sampler2D u_DepthTexture;

const float kPI = 3.14159265359;

struct AtmosphereParameters
{
	// Radius of the planet (center to ground)
	float BottomRadius;

	// Maximum considered atmosphere height (center to atmosphere top)
	float TopRadius;

	// Rayleigh scattering exponential distribution scale in the atmosphere
	float RayleighDensityExpScale;

	// Rayleigh scattering coefficients
	vec3 RayleighScattering;

	// Mie scattering exponential distribution scale in the atmosphere
	float MieDensityExpScale;

	// Mie scattering coefficients
	vec3 MieScattering;

	// Mie extinction coefficients
	vec3 MieExtinction;

	// Mie absorption coefficients
	vec3 MieAbsorption;

	// Mie phase function excentricity
	float MiePhaseG;

	// Another medium type in the atmosphere
	float AbsorptionDensity0LayerWidth;
	float AbsorptionDensity0ConstantTerm;
	float AbsorptionDensity0LinearTerm;
	float AbsorptionDensity1ConstantTerm;
	float AbsorptionDensity1LinearTerm;

	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	vec3 AbsorptionExtinction;

	// The albedo of the ground.
	vec3 GroundAlbedo;

	float MultipleScatteringFactor;
	uint ViewRayMarchMinSPP;
	uint ViewRayMarchMaxSPP;
};

struct AtmosphereParametersIn 
{
	vec3  AbsorptionColor;
	float AbsorptionLength;

	vec3  RayleighScatteringColor;
	float RayleighScatterLength;

	vec3  MieScatteringColor;
	float MieScatteringLength;

	vec3  MieAbsColor;
	float MieAbsLength;

	vec3 MieAbsorption;
	int  ViewRayMarchMinSPP;

	vec3 GroundAlbedo;
	int  ViewRayMarchMaxSPP;

	vec4 RayleighDensity[ 3 ];
	vec4 MieDensity[ 3 ];
	vec4 AbsorptionDensity[ 3 ];

	float MultipleScatteringFactor;
	float MiePhaseFunctionG;
	float BottomRadius;
	float TopRadius;
};

layout(binding = 6) uniform AtmosParam
{
	AtmosphereParametersIn AtmosphereInData;
} u_AtmosphereParameters;

layout(binding = 7) uniform CameraData
{
	mat4 InverseViewProj;
} u_CameraData;

AtmosphereParameters GetAtmosphereParameters() 
{
	AtmosphereParameters atmosphereParameters;
	const AtmosphereParametersIn inputData = u_AtmosphereParameters.AtmosphereInData;

	atmosphereParameters.AbsorptionExtinction = inputData.AbsorptionColor * inputData.AbsorptionLength;

	atmosphereParameters.GroundAlbedo = inputData.GroundAlbedo;
	atmosphereParameters.BottomRadius = inputData.BottomRadius;
	atmosphereParameters.TopRadius    = inputData.TopRadius;
	atmosphereParameters.ViewRayMarchMinSPP = inputData.ViewRayMarchMinSPP;
	atmosphereParameters.ViewRayMarchMaxSPP = inputData.ViewRayMarchMaxSPP;
	atmosphereParameters.MiePhaseG			= inputData.MiePhaseFunctionG;
	atmosphereParameters.MultipleScatteringFactor = inputData.MultipleScatteringFactor;

	// Traslation from Bruneton2017 parameterisation.
	atmosphereParameters.RayleighDensityExpScale        = inputData.RayleighDensity[ 1 ].w;
	atmosphereParameters.MieDensityExpScale             = inputData.MieDensity[ 1 ].w;
	atmosphereParameters.AbsorptionDensity0LayerWidth   = inputData.AbsorptionDensity[ 0 ].x;
	atmosphereParameters.AbsorptionDensity0ConstantTerm = inputData.AbsorptionDensity[ 1 ].x;
	atmosphereParameters.AbsorptionDensity0LinearTerm   = inputData.AbsorptionDensity[ 0 ].w;
	atmosphereParameters.AbsorptionDensity1ConstantTerm = inputData.AbsorptionDensity[ 2 ].y;
	atmosphereParameters.AbsorptionDensity1LinearTerm   = inputData.AbsorptionDensity[ 2 ].x;

	atmosphereParameters.RayleighScattering = inputData.RayleighScatteringColor * inputData.RayleighScatterLength;
	atmosphereParameters.MieAbsorption      = inputData.MieAbsorption;
	atmosphereParameters.MieScattering      = inputData.MieScatteringColor * inputData.MieScatteringLength;
	atmosphereParameters.MieExtinction      = atmosphereParameters.MieScattering + inputData.MieAbsColor * inputData.MieAbsLength;

	return atmosphereParameters;
}

float RayleighPhase( float cosTheta ) 
{
	const float factor = 3.0f / ( 16.0f * kPI );
	return factor * ( 1.0f + cosTheta * cosTheta );
}

float HenyeyGreenstein( float cosTheta, float g )
{
	float gg = g * g;
	return ( 1 - gg ) / pow( 1.0 + gg - 2 * g * cosTheta, 1.5 );
}

float GetUniformPhase()
{
	return 1.0f / ( 4.0f * kPI );
}

float HGPhase( float g, float cosTheta ) 
{
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;

	return numer / ( 4.0f * kPI * denom * sqrt( denom ) );
}

vec3 ConstructPosition( vec2 uv, float depthZ, mat4 m ) 
{
	vec4 clipPos = vec4( uv.x * 2.0f - 1.0f, 1.0f - uv.y, depthZ, 1.0f );
	vec4 posWorld = m * clipPos;
	
	return posWorld.xyz / posWorld.w;
}

vec3 GetWorldPosition( vec2 uv, float depthZ ) 
{
	return ConstructPosition( uv, depthZ, u_CameraData.InverseViewProj );
}

/**
* - RayIntersectsSphereNearest
*
*   References used: TODO
* 
*/
float RayIntersectsSphereNearest( vec3 origin, vec3 direction, vec3 sphereCentre, float sphereRadius ) 
{
	float a = dot( direction, direction );
	vec3 s0r0 = origin - sphereCentre;
	float b = 2.0 * dot( direction, s0r0 );
	float c = dot( s0r0, s0r0 ) - ( sphereRadius * sphereRadius );
	float delta = b * b - 4.0 * a * c;
	
	if( delta < 0.0 || a == 0.0 )
		return -1.0;

	float sol0 = ( -b - sqrt( delta ) ) / ( 2.0 * a );
	float sol1 = ( -b + sqrt( delta ) ) / ( 2.0 * a );

	if( sol0 < 0.0 && sol1 < 0.0 )
	{
		return -1.0;
	}

	if( sol0 < 0.0 )
		return max( 0.0, sol1 );
	else if( sol1 < 0.0 )
		return max( 0.0, sol0 );

	return max( 0.0, min( sol0, sol1 ) );
}

float RaySphereIntersectInside( vec3 r0, vec3 rd, vec3 s0,float sR )
{
	float a = dot( rd, rd );

	vec3 s02r0 = r0 - s0;
	float b = 2.0 * dot( rd, s02r0 );

	float c = dot( s02r0, s02r0 ) - ( sR * sR );
	float delta = b * b - 4.0 * a * c;

	// float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);

	// sol1 > sol0, so just return sol1
	return sol1;
}

bool RaySphereIntersectOutSide( vec3 r0, vec3 rd, vec3 s0, float sR, out vec2 t0t1 ) 
{
	float a = dot(rd, rd);

	vec3 s02r0 = r0 - s0;
	float b = 2.0 * dot(rd, s02r0);

	float c = dot(s02r0, s02r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;

    // No intersection state.
	if (delta < 0.0 || a == 0.0)
	{
		return false;
	}

	float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);
	

    // Intersection on negative direction, no suitable for ray.
	if (sol1 <= 0.0 || sol0 <= 0.0)
	{
		return false;
	}

    // Two positive intersection, return nearest one.
	t0t1 = vec2(sol0, sol1); // sol1 > sol0
	return true; 
}

struct MediumSampleRGB
{
	vec3 Scattering;
	vec3 Absorption;
	vec3 Extinction;

	vec3 ScatteringMie;
	vec3 AbsorptionMie;
	vec3 ExtinctionMie;

	vec3 ScatteringRay;
	vec3 AbsorptionRay;
	vec3 ExtinctionRay;

	vec3 ScatteringOzo;
	vec3 AbsorptionOzo;
	vec3 ExtinctionOzo;

	vec3 Albedo;
};

float GetAlbedo( float scattering, float extinction )
{
	return scattering / max( 0.001, extinction );
}

vec3 GetAlbedo( vec3 scattering, vec3 extinction )
{
	return scattering / max( vec3( 0.001 ), extinction );
}

float GetViewHeight( vec3 worldPos, const AtmosphereParameters atmosphere )
{
	return length( worldPos ) - atmosphere.BottomRadius;
}

MediumSampleRGB SampleMediumRGB( vec3 worldPos, const AtmosphereParameters atmosphere )
{
	const float viewHeight = GetViewHeight( worldPos, atmosphere );

	const float densityMie = exp( atmosphere.MieDensityExpScale * viewHeight );
	const float densityRay = exp( atmosphere.RayleighDensityExpScale * viewHeight );

	// Ozone layer.
	const float densityOzo = clamp( 
							viewHeight < atmosphere.AbsorptionDensity0LayerWidth ? 
							atmosphere.AbsorptionDensity0LinearTerm * viewHeight + atmosphere.AbsorptionDensity0ConstantTerm :
							atmosphere.AbsorptionDensity1LinearTerm * viewHeight + atmosphere.AbsorptionDensity1ConstantTerm, 0.0, 1.0 );
	
	MediumSampleRGB s;
	// Mie term.
	s.ScatteringMie = densityMie * atmosphere.MieScattering;
	s.AbsorptionMie = densityMie * atmosphere.MieAbsorption;
	s.ExtinctionMie = densityMie * atmosphere.MieExtinction;

	// Ray term.
	s.ScatteringRay = densityRay * atmosphere.RayleighScattering;
	s.AbsorptionRay = vec3(0.0);
	s.ExtinctionRay = s.ScatteringRay + s.AbsorptionRay;

	// Ozone term.
	s.ScatteringOzo = vec3(0.0);
	s.AbsorptionOzo = densityOzo * atmosphere.AbsorptionExtinction;
	s.ExtinctionOzo = s.ScatteringOzo + s.AbsorptionOzo;

	// Composite.
	s.Scattering = s.ScatteringMie + s.ScatteringRay + s.ScatteringOzo;
	s.Absorption = s.AbsorptionMie + s.AbsorptionRay + s.AbsorptionOzo;
	s.Extinction = s.ExtinctionMie + s.ExtinctionRay + s.ExtinctionOzo;
	s.Albedo = GetAlbedo( s.Scattering, s.Extinction );

	return s;
}

void LutTransmittanceParamsToUv( 
					AtmosphereParameters atmosphere,
					float viewHeight, // [bottomRadius, topRadius]
					float viewZenithCosAngle, // [-1, 1]
					out vec2 uv )
{
	float H = sqrt( max( 0.0f, atmosphere.TopRadius * atmosphere.TopRadius - atmosphere.BottomRadius * atmosphere.BottomRadius ) );
	float rho = sqrt( max( 0.0f, viewHeight * viewHeight - atmosphere.BottomRadius * atmosphere.BottomRadius ) );

	uv.y = rho / H;

	float discriminant = viewHeight * viewHeight * ( viewZenithCosAngle * viewZenithCosAngle - 1.0 ) + atmosphere.TopRadius * atmosphere.TopRadius;
	float d = max( 0.0, ( -viewHeight * viewZenithCosAngle + sqrt( discriminant ) ) );

	float dMin = atmosphere.TopRadius - viewHeight;
	float dMax = rho + H;

	uv.x = ( d - dMin ) / ( dMax - dMin );
}

struct SingleScatteringResult
{
	vec3 ScatteredLight; // Scattered light (luminance)
	vec3 OpticalDepth;   // Optical depth (1/m)
	vec3 Transmittance;  // Transmittance in [0,1] (unitless)
	vec3 MultiScatAs1;

	vec3 NewMultiScatStep0Out;
	vec3 NewMultiScatStep1Out;
};

SingleScatteringResult SingleScatteringResult_Create() 
{
	SingleScatteringResult ss;
	ss.ScatteredLight       = vec3( 0.0 );
	ss.OpticalDepth         = vec3( 0.0 );
	ss.Transmittance        = vec3( 0.0 );
	ss.MultiScatAs1         = vec3( 0.0 );
	ss.NewMultiScatStep0Out = vec3( 0.0 );
	ss.NewMultiScatStep1Out = vec3( 0.0 );

	return ss;
}

const float kDefaultMaxT = 9000000.0f;
const float kAtmospherePlanetRadiusOffset = 21.0f * 0.001f;

float GetShadow( AtmosphereParameters atmosphere, vec3 p )
{
	return 1.0f;
}

#if defined( ATMOSPHERE_MULTISCATTERING )
layout (set = 0, binding = 13) uniform sampler2D u_MultiScatteringLut;

vec3 GetMultipleScattering( AtmosphereParameters atmosphere, vec3 worldPos, float viewZenithCosAngle )
{
	// Inverse of the mapping used to write the LUT (edge texels on 0 and 1).
	float heightRange = atmosphere.TopRadius - atmosphere.BottomRadius - kAtmospherePlanetRadiusOffset;
	vec2 u = clamp( vec2(
		viewZenithCosAngle * 0.5 + 0.5,
		( length( worldPos ) - atmosphere.BottomRadius - kAtmospherePlanetRadiusOffset ) / heightRange ), 0.0, 1.0 );

	// Image2D samplers are NEAREST, filter by hand.
	ivec2 size = textureSize( u_MultiScatteringLut, 0 );
	ivec2 maxP = size - 1;
	vec2 p = u * vec2( maxP );
	ivec2 i = min( ivec2( floor( p ) ), maxP );
	vec2 f = p - vec2( i );

	vec3 a = texelFetch( u_MultiScatteringLut, i, 0 ).rgb;
	vec3 b = texelFetch( u_MultiScatteringLut, min( i + ivec2( 1, 0 ), maxP ), 0 ).rgb;
	vec3 c = texelFetch( u_MultiScatteringLut, min( i + ivec2( 0, 1 ), maxP ), 0 ).rgb;
	vec3 d = texelFetch( u_MultiScatteringLut, min( i + ivec2( 1, 1 ), maxP ), 0 ).rgb;

	return mix( mix( a, b, f.x ), mix( c, d, f.x ), f.y );
}
#endif

SingleScatteringResult IntegrateScatteredLuminance( 
								vec2 pixPos, 
								vec3 worldPos, 
								vec3 worldDir,
								vec3 sunDir,
								AtmosphereParameters atmosphere,
								bool ground,
								float sampleCountIni,
								float depthBufferValue,
								bool mieRayPhase,
								float tMaxMax,
								bool variableSampleCount ) 
{
	SingleScatteringResult ss = SingleScatteringResult_Create();

	const vec3 kEarthOrigin = vec3( 0.0 );

	float tBottom = RayIntersectsSphereNearest( worldPos, worldDir, kEarthOrigin, atmosphere.BottomRadius );
	float tTop = RayIntersectsSphereNearest( worldPos, worldDir, kEarthOrigin, atmosphere.TopRadius );

	float tMax = 0.0;

	if( tBottom < 0.0 )
	{
		if( tTop < 0.0 )
		{
			// No intersection with earth nor atmosphere: stop right away 
			return ss;
		}
		else
		{
			tMax = tTop;
		}
	}
	else
	{
		if( tTop > 0.0 )
		{
			tMax = min( tTop, tBottom );
		}
	}

	if( depthBufferValue >= 0.0 && depthBufferValue <= 1.0 )
	{
		vec2 sampleUv = ( pixPos + vec2( 0.5 ) ) / vec2( textureSize( u_DepthTexture, 0 ) );
		vec3 depthBufferwWorldPosition = GetWorldPosition( sampleUv, depthBufferValue );

		float tDepth = length( depthBufferwWorldPosition * 0.001 + vec3( 0.0, atmosphere.BottomRadius, 0.0 ) - worldPos );

		if( tDepth < tMax )
		{
			tMax = tDepth;
		}
	}
	tMax = min( tMax, tMaxMax );

	// Sample Count
	float sampleCount = sampleCountIni;
	float sampleCountFloor = sampleCountIni;
	float tMaxFloor = tMax;

	if( variableSampleCount ) 
	{
		sampleCount = mix( float( atmosphere.ViewRayMarchMinSPP ), float( atmosphere.ViewRayMarchMaxSPP ), clamp( tMax * 0.01, 0.0, 1.0 ) );
		sampleCountFloor = floor( sampleCount );

		tMaxFloor = tMax * sampleCountFloor / sampleCount;
	}

	float dt = tMax / sampleCount;

	const float uniformPhase = GetUniformPhase();
	const vec3 wi = sunDir;
	const vec3 wo = worldDir;
	
	float cosTheta = dot( wi, wo );
	float miePhaseValue = HGPhase( atmosphere.MiePhaseG, -cosTheta );

	float rayleighPhaseValue = RayleighPhase( cosTheta );

	vec3 globalL = vec3( 1.0 );

	vec3 L = vec3( 0.0f );
	vec3 throughput = vec3( 1.0 );
	vec3 opticalDepth = vec3( 0.0 );

	float t = 0.0;
	float tPrev = 0.0;
	
	const float kSampleSegmentT = 0.3f;

	for( float s = 0.0f; s < sampleCount; s += 1.0f ) 
	{
		if( variableSampleCount )
		{
			float t0 = s / sampleCountFloor;
			float t1 = s + 1.0 / sampleCountFloor;

			t0 = t0 * t0;
			t1 = t1 * t1;

			t0 = tMaxFloor * t0;
			if( t1 > 1.0 )
			{
				t1 = tMax;
			}
			else
			{
				t1 = tMaxFloor * t1;
			}

			t = t0 + ( t1 - t0 ) * kSampleSegmentT;
			dt = t1 - t0;
		}
		else
		{
			float newT = tMax * ( s + kSampleSegmentT ) / sampleCount;
			dt = newT - t;
			t = newT;
		}

		vec3 P = worldPos + t * worldDir;

		MediumSampleRGB medium = SampleMediumRGB( P, atmosphere );

		const vec3 sampleOpticalDepth = medium.Extinction * dt;

		const vec3 sampleTransmittance = exp( -sampleOpticalDepth );
		opticalDepth += sampleOpticalDepth;

		float pHeight = length( P );
		const vec3 upVector = P / pHeight;

		float sunZenithCosAngle = dot( sunDir, upVector );
		vec2 uv;
		LutTransmittanceParamsToUv( atmosphere, pHeight, sunZenithCosAngle, uv );

		vec3 transmittanceToSun = texture( u_TransmittanceLut, uv ).rgb;

		vec3 phaseTimesScattering;
		if( mieRayPhase ) 
		{
			phaseTimesScattering = medium.ScatteringMie * miePhaseValue + medium.ScatteringRay * rayleighPhaseValue;
		}
		else
		{
			phaseTimesScattering = medium.Scattering * uniformPhase;
		}

		// Shadow from earth.
		float tEarth = RayIntersectsSphereNearest( P, sunDir, kEarthOrigin + kAtmospherePlanetRadiusOffset * upVector, atmosphere.BottomRadius );
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

#if defined( ATMOSPHERE_MULTISCATTERING )
		vec3 multiScatteredLuminance = GetMultipleScattering( atmosphere, P, sunZenithCosAngle );
#else
		vec3 multiScatteredLuminance = vec3( 0.0 );
#endif

		float shadow = GetShadow( atmosphere, P );

		vec3 S = globalL * ( earthShadow * shadow * transmittanceToSun * phaseTimesScattering + ( multiScatteredLuminance * medium.Scattering ) );

		vec3 ms = medium.Scattering * 1;
		vec3 msint = ( ms - ms * sampleTransmittance ) / medium.Extinction;
		ss.MultiScatAs1 += throughput * msint;

		// Evaluate input to multi scattering
		{
			vec3 newMS;

			newMS = earthShadow * transmittanceToSun * medium.Scattering * uniformPhase * 1;
			ss.NewMultiScatStep0Out += throughput * ( newMS - newMS * sampleTransmittance ) / medium.Extinction;

			newMS = medium.Scattering * uniformPhase * multiScatteredLuminance;
			ss.NewMultiScatStep1Out += throughput * ( newMS - newMS * sampleTransmittance ) / medium.Extinction;
		}

		vec3 Sint = ( S - S * sampleTransmittance ) / medium.Extinction;
		L += throughput * Sint;
		throughput *= sampleTransmittance;

		tPrev = t;
	}

	if( ground && tMax == tBottom && tBottom > 0.0 )
	{
		vec3 P = worldPos + tBottom * worldDir;
		float pHeight = length( P );

		const vec3 upVector = P / pHeight;
		float sunZenithCosAngle = dot( sunDir, upVector );
		vec2 uv;
		LutTransmittanceParamsToUv( atmosphere, pHeight, sunZenithCosAngle, uv );

		vec3 transmittanceToSun = texture( u_TransmittanceLut, uv ).rgb;

		const float NdotL = clamp( dot( normalize( upVector ), normalize( sunDir ) ), 0.0, 1.0 );
		L += globalL * transmittanceToSun * throughput * NdotL * atmosphere.GroundAlbedo / kPI;
	}

	ss.ScatteredLight = L;
	ss.OpticalDepth = opticalDepth;
	ss.Transmittance = throughput;

	return ss;
}
