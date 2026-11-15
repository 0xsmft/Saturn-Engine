//
// SkyAtmosphereTransmittanceLut
//
// The following shader code is based from:
//
// Sebastien Hillaire: https://github.com/sebh/UnrealEngineSkyAtmosphere
//

#type compute
#version 450 core

#include <AtmosphereCommon.glsl>

layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D o_OutTransmittanceLut;

layout(push_constant) uniform Params
{
	vec3 SunDir;
} u_SkyData;

void UvToLutTransmittanceParams( 
					AtmosphereParameters atmos, 
					out float viewHeight, 
					out float viewZenithCosAngle, 
					vec2 uv ) 
{
	float H = sqrt( atmos.TopRadius * atmos.TopRadius - atmos.BottomRadius * atmos.BottomRadius );
	float rho = H * uv.y;

	viewHeight = sqrt( rho * rho + atmos.BottomRadius * atmos.BottomRadius );

	float dMin = atmos.TopRadius - viewHeight;
	float dMax = rho + H;

	float d = dMin + uv.x * ( dMax - dMin );

	viewZenithCosAngle = ( d == 0.0 ) ? 1.0f : ( H * H - rho * rho - d * d ) / ( 2.0 * viewHeight * d );
	viewZenithCosAngle = clamp( viewZenithCosAngle, -1.0, 1.0 );
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 lutSize = imageSize( o_OutTransmittanceLut );
	ivec2 workPos = ivec2( gl_GlobalInvocationID.xy );

	AtmosphereParameters atmos = GetAtmosphereParameters();

	const vec2 pixPos = vec2( workPos ) + vec2( 0.5f );
	const vec2 uv = pixPos / vec2( lutSize );

	float viewHeight;
	float viewZenithCosAngle;
	UvToLutTransmittanceParams( atmos, viewHeight, viewZenithCosAngle, uv );
	
	const vec3 worldPos = vec3( 0.0f, viewHeight, 0.0f );
	const vec3 worldDir = vec3( 0.0f, viewZenithCosAngle, sqrt ( 1.0 - viewZenithCosAngle * viewZenithCosAngle ) );
	const vec3 sunDir = -normalize( u_SkyData.SunDir );
	const bool bGround = false;
	const float sampleCountIni = 40.0;
	const float depthBufferValue = -1.0;
	const bool bMieRayPhase = false;
	const float tMaxMax = kDefaultMaxT;
	const bool bVariableSampleCount = false;
	vec3 opticalDepth = IntegrateScatteredLuminance(
		pixPos, 
		worldPos, 
		worldDir, 
		sunDir, 
		atmos,
		bGround, 
		sampleCountIni, 
		depthBufferValue, 
		bMieRayPhase, 
		tMaxMax,
		bVariableSampleCount
	).OpticalDepth;

	vec3 transmittance = exp( -opticalDepth );
	transmittance = min( transmittance, vec3( 65504.0f ) );
	imageStore( o_OutTransmittanceLut, workPos, vec4( transmittance, 1.0 ) );
}
