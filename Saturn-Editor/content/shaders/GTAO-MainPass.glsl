// GTAO (Ground-Truth Ambient Occlusion) Main pass
//
// This implementation is based from the following items:
//
// - Erin Catto, box3d (https://github.com/erincatto/box3d), MIT
// - Bobby Anguelov, Esoterica Engine (https://github.com/BobbyAnguelov/Esoterica), MIT
// - Intel Corporation, XeGTAO (https://github.com/GameTechDev/XeGTAO), MIT
//

#type compute
#version 460

layout(set = 0, binding = 0) uniform texture2D u_InDepthPreDepth; // Hardware depth
layout(set = 0, binding = 1) uniform texture2D u_InDepthGTAO;	  // GTAO prefilterd depth
layout(set = 0, binding = 2) uniform sampler   s_PointSampler;

layout(set = 0, binding = 3, r8)   uniform writeonly image2D o_Edges;
layout(set = 0, binding = 4, r32f) uniform writeonly image2D o_NoisyGTAO;

layout(push_constant) uniform Params 
{
	float EffectRadius;
	float EffectFalloffRange;
	float RadiusMultiplier;
	vec2 NDCToViewMul_x_PixelSize;
	float FinalValuePower;
	float SampleDistributionPower;
	float ThinOccluderCompensation;
	float DepthMipSamplingOffset;
	float SliceCount;
	float StepsPerSlice;
} pc_Params;

layout(binding = 5) uniform NdcData
{
	ivec2 ViewportSize;

	// Size of one texel (x,y) so the maths is 1.0 / ViewportSize 
	vec2 ViewportPixelSize;
	vec2 NDCToViewMul;
	vec2 NDCToViewAdd;
	vec2 DepthUnpackConsts;
} u_ExtraData;

struct GTAOConsts
{
	vec2 ViewportPixelSize;
	vec2 NDCToViewMul;
	vec2 NDCToViewAdd;
	vec2 NDCToViewMul_x_PixelSize;
	float EffectRadius;
	float EffectFalloffRange;
	float RadiusMultiplier;
	float FinalValuePower;
	float SampleDistributionPower;
	float ThinOccluderCompensation;
	float DepthMIPSamplingOffset;
	float SliceCount;
	float StepsPerSlice;
};

const uint XE_HILBERT_WIDTH = 64u;
const float XE_GTAO_DEPTH_MIP_LEVELS = 5.0;
const float XE_GTAO_OCCLUSION_TERM_SCALE = 1.5;
const float XE_GTAO_PI = 3.1415926535897932384626433832795;
const float XE_GTAO_PI_HALF = 1.5707963267948966192313216916398;

float XeGTAO_FastSqrt( float x )
{
	return intBitsToFloat( 0x1fbd1df5 + ( floatBitsToInt( x ) >> 1 ) );
}

float XeGTAO_FastACos( float inX )
{
	float x = abs( inX );
	float res = -0.156583 * x + XE_GTAO_PI_HALF;
	res *= XeGTAO_FastSqrt( 1.0 - x );
	return ( inX >= 0.0 ) ? res : ( XE_GTAO_PI - res );
}

float XeGTAO_ScreenSpaceToViewSpaceDepth( float screenDepth, GTAOConsts consts )
{
	return abs( u_ExtraData.DepthUnpackConsts.x / ( screenDepth + u_ExtraData.DepthUnpackConsts.y ) );
}

vec3 XeGTAO_ComputeViewspacePosition( vec2 screenPos, float viewspaceDepth, GTAOConsts consts )
{
	vec3 ret;
	ret.xy = fma( consts.NDCToViewMul, screenPos, consts.NDCToViewAdd ) * viewspaceDepth;
	ret.z = -viewspaceDepth;
	return ret;
}

vec4 XeGTAO_CalculateEdges( float centerZ, float leftZ, float rightZ, float topZ, float bottomZ )
{
	vec4 edgesLRTB = vec4( leftZ, rightZ, topZ, bottomZ ) - centerZ;
	
	float slopeLR = ( edgesLRTB.y - edgesLRTB.x ) * 0.5;
	float slopeTB = ( edgesLRTB.w - edgesLRTB.z ) * 0.5;
	vec4 edgesLRTBSlopeAdjusted = edgesLRTB + vec4( slopeLR, -slopeLR, slopeTB, -slopeTB );
	edgesLRTB = min( abs( edgesLRTB ), abs( edgesLRTBSlopeAdjusted ) );
	return vec4( clamp( ( 1.25 - edgesLRTB / ( centerZ * 0.011 ) ), 0.0, 1.0 ) );
}

float XeGTAO_PackEdges( vec4 edgesLRTB )
{
	edgesLRTB = round( clamp( edgesLRTB, 0.0, 1.0 ) * 2.9 );
	return dot( edgesLRTB, vec4( 64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0 ) );
}

vec4 XeGTAO_UnpackEdges( float packedVal )
{
	uint p = uint( packedVal * 255.5 );
	vec4 edgesLRTB;
	edgesLRTB.x = float( ( p >> 6 ) & 0x03u ) / 3.0;
	edgesLRTB.y = float( ( p >> 4 ) & 0x03u ) / 3.0;
	edgesLRTB.z = float( ( p >> 2 ) & 0x03u ) / 3.0;
	edgesLRTB.w = float( ( p >> 0 ) & 0x03u ) / 3.0;
	return clamp( edgesLRTB, 0.0, 1.0 );
}

vec3 XeGTAO_CalculateNormal(
	vec4 edgesLRTB, 
	vec3 pixCenterPos,
	vec3 pixLPos, 
	vec3 pixRPos, 
	vec3 pixTPos, 
	vec3 pixBPos )
{
	vec4 acceptedNormals = clamp( vec4(
			edgesLRTB.x * edgesLRTB.z,
			edgesLRTB.z * edgesLRTB.y,
			edgesLRTB.y * edgesLRTB.w,
		edgesLRTB.w * edgesLRTB.x ) + 0.01, 0.0, 1.0 );
		
	pixLPos = normalize( pixLPos - pixCenterPos );
	pixRPos = normalize( pixRPos - pixCenterPos );
	pixTPos = normalize( pixTPos - pixCenterPos );
	pixBPos = normalize( pixBPos - pixCenterPos );
	
	vec3 pixelNormal = 
		  acceptedNormals.x * cross( pixTPos, pixLPos )
		+ acceptedNormals.y * cross( pixRPos, pixTPos )
		+ acceptedNormals.z * cross( pixBPos, pixRPos )
		+ acceptedNormals.w * cross( pixLPos, pixBPos );

	return normalize( pixelNormal );
}

uint HilbertIndex(uint posX, uint posY)
{
	uint index = 0u;
	for (uint curLevel = XE_HILBERT_WIDTH / 2u; curLevel > 0u; curLevel /= 2u)
	{
		uint regionX = (posX & curLevel) > 0u ? 1u : 0u;
		uint regionY = (posY & curLevel) > 0u ? 1u : 0u;
		index += curLevel * curLevel * ((3u * regionX)^ regionY);
		if (regionY == 0u)
		{
			if (regionX == 1u)
			{
				posX = (XE_HILBERT_WIDTH - 1u) - posX;
				posY = (XE_HILBERT_WIDTH - 1u) - posY;
			}
			uint temp = posX;
			posX = posY;
			posY = temp;
		}
	}
	return index;
}

vec2 SpatioTemporalNoise( uvec2 pixCoord, int temporalIndex )
{
	uint index = HilbertIndex( pixCoord.x, pixCoord.y );
	index += 288u * ( uint( temporalIndex ) % 64u );

	// R2 sequence, Roberts 2018.
	return fract( 0.5 + float( index ) * vec2( 0.75487766624669276005, 0.5698402909980532659114 ) );
}

vec3 XeGTAO_ComputeViewspaceNormal( ivec2 pixCoord, GTAOConsts consts )
{
	vec2 normalisedScreenPos = ( vec2( pixCoord ) + 0.5 ) * consts.ViewportPixelSize;

	// Clamp the 4 cardinal taps into the viewport. At the viewport edge
	// we replicate the center pixel, same approximation as gather-with-
	// sampler-clamp.
	ivec2 vmax = u_ExtraData.ViewportSize - 1;
	ivec2 cC = clamp( pixCoord, ivec2( 0 ), vmax );
	ivec2 cL = clamp( pixCoord + ivec2( -1, 0 ), ivec2( 0 ), vmax );
	ivec2 cR = clamp( pixCoord + ivec2( 1, 0 ), ivec2( 0 ), vmax );
	ivec2 cT = clamp( pixCoord + ivec2( 0, -1 ), ivec2( 0 ), vmax );
	ivec2 cB = clamp( pixCoord + ivec2( 0, 1 ), ivec2( 0 ), vmax );

	float viewspaceZ = XeGTAO_ScreenSpaceToViewSpaceDepth( texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), cC, 0 ).r, consts );
	
	float pixLZ = XeGTAO_ScreenSpaceToViewSpaceDepth( texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), cL, 0 ).r, consts );
	float pixRZ = XeGTAO_ScreenSpaceToViewSpaceDepth( texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), cR, 0 ).r, consts );
	float pixTZ = XeGTAO_ScreenSpaceToViewSpaceDepth( texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), cT, 0 ).r, consts );
	float pixBZ = XeGTAO_ScreenSpaceToViewSpaceDepth( texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), cB, 0 ).r, consts );

	vec4 edgesLRTB = XeGTAO_CalculateEdges( viewspaceZ, pixLZ, pixRZ, pixTZ, pixBZ );

	vec3 CENTRE = XeGTAO_ComputeViewspacePosition( normalisedScreenPos,												    viewspaceZ, consts );
	vec3 LEFT   = XeGTAO_ComputeViewspacePosition( normalisedScreenPos + vec2( -1.0,  0.0 ) * consts.ViewportPixelSize, pixLZ, consts );
	vec3 RIGHT  = XeGTAO_ComputeViewspacePosition( normalisedScreenPos + vec2(  1.0,  0.0 ) * consts.ViewportPixelSize, pixRZ, consts );
	vec3 TOP    = XeGTAO_ComputeViewspacePosition( normalisedScreenPos + vec2(  0.0, -1.0 ) * consts.ViewportPixelSize, pixTZ, consts );
	vec3 BOTTOM = XeGTAO_ComputeViewspacePosition( normalisedScreenPos + vec2(  0.0,  1.0 ) * consts.ViewportPixelSize, pixBZ, consts );

	return XeGTAO_CalculateNormal( edgesLRTB, CENTRE, LEFT, RIGHT, TOP, BOTTOM );
}

float XeGTAO_OutputWorkingTerm( ivec2 pixCoord, float visibility )
{
	visibility = clamp( visibility / XE_GTAO_OCCLUSION_TERM_SCALE, 0.0, 1.0 );
	imageStore( o_NoisyGTAO, pixCoord, vec4( visibility, 0.0, 0.0, 0.0 ) );
	return visibility;
}

float XeGTAO_MainPass( ivec2 pixCoord, 
					   vec2 localNoise, 
					   vec3 viewspaceNormal,
					   float centerViewspaceZ,
					   vec4 edgesLRTB,
					   GTAOConsts consts )
{
	vec2 normalisedScreenPos = ( vec2( pixCoord ) + 0.5 ) * consts.ViewportPixelSize;
	float viewspaceZ = centerViewspaceZ;
	viewspaceZ *= 0.99999;

	// Store edges
	imageStore( o_Edges, pixCoord, vec4( XeGTAO_PackEdges( edgesLRTB ), 0.0, 0.0, 0.0 ) );

	vec3 pixCentrePos = XeGTAO_ComputeViewspacePosition( normalisedScreenPos, viewspaceZ, consts );
	vec3 viewVec = normalize( -pixCentrePos );

	float sliceCount = consts.SliceCount;
	float stepsPerSlice = consts.StepsPerSlice;
	const float effectRadius = consts.EffectRadius * consts.RadiusMultiplier;
	const float falloffRange = consts.EffectFalloffRange * effectRadius;
	const float falloffFrom = effectRadius * ( 1.0 - consts.EffectFalloffRange );
	const float falloffMul = -1.0 / falloffRange;
	const float falloffAdd = falloffFrom / falloffRange + 1.0;
	const float sampleDistributionPower = consts.SampleDistributionPower;
	const float thinOccluderCompensation = consts.ThinOccluderCompensation;

	float visibility = 0.0;

	{
		const float noiseSlice = localNoise.x;
		const float noiseSample = localNoise.y;
		const float pixelTooCloseThreshold = 1.3;

		vec2 pixelDirRBViewspaceSizeAtCenterZ = vec2( viewspaceZ, viewspaceZ ) * consts.NDCToViewMul_x_PixelSize;
		float screenspaceRadius = effectRadius / pixelDirRBViewspaceSizeAtCenterZ.x;

		// Fade out for small screen radii, XeGTAO.esh:382.
		visibility += clamp( ( 10.0 - screenspaceRadius ) / 100.0, 0.0, 1.0 ) * 0.5;

		if( screenspaceRadius < pixelTooCloseThreshold )
		{
			return XeGTAO_OutputWorkingTerm( pixCoord, 1 );
		}

		const float minS = pixelTooCloseThreshold / screenspaceRadius;
		for( float slice = 0.0; slice < sliceCount; slice += 1.0 )
		{
			float sliceK = ( slice + noiseSlice ) / sliceCount;
			float phi = sliceK * XE_GTAO_PI;
			float cosPhi = cos( phi );
			float sinPhi = sin( phi );
			vec2 omega = vec2( cosPhi, -sinPhi );
			omega *= screenspaceRadius;

			vec3 directionVec = vec3( cosPhi, sinPhi, 0.0 );
			vec3 orthoDirectionVec = directionVec - ( dot( directionVec, viewVec ) * viewVec );
			vec3 axisVec = normalize( cross( orthoDirectionVec, viewVec ) );

			vec3 projectedNormalVec = viewspaceNormal - axisVec * dot( viewspaceNormal, axisVec );

			float signNorm = sign( dot( orthoDirectionVec, projectedNormalVec ) );
			float projectedNormalVecLength = length( projectedNormalVec );
			float cosNorm = clamp( dot( projectedNormalVec, viewVec ) / projectedNormalVecLength, 0.0, 1.0 );

			float n = signNorm * XeGTAO_FastACos( cosNorm );

			const float lowHorizonCos0 = cos( n + XE_GTAO_PI_HALF );
			const float lowHorizonCos1 = cos( n - XE_GTAO_PI_HALF );

			float horizonCos0 = lowHorizonCos0;
			float horizonCos1 = lowHorizonCos1;

			for( float step_ = 0.0; step_ < stepsPerSlice; step_ += 1.0 )
			{
				// R1 sequence.
				float stepBaseNoise = ( slice + step_ * stepsPerSlice ) * 0.6180339887498948482;
				float stepNoise = fract( noiseSample + stepBaseNoise );

				float s = ( step_ + stepNoise ) / stepsPerSlice;
				s = pow( s, sampleDistributionPower );
				s += minS;

				vec2 sampleOffset = s * omega;
				float sampleOffsetLength = length( sampleOffset );

				const float mipCount = log2( sampleOffsetLength ) - consts.DepthMIPSamplingOffset;
				const float mipLevel = clamp( mipCount, 0.0, XE_GTAO_DEPTH_MIP_LEVELS );

				// TODO: This is hack, but like lowkey it works so...
				// like this shit should be here but for some reason mipLevel just doesnt get clamped or something
				// because when we go super close to an object and without this check, the object would turn black,
				// with this check the object is fine.
				// but even so, mipLevel should be clamped to 5.0, because we only have 5 mips. Black magic.
				if( mipLevel >= 5.0 )
				{
					return XeGTAO_OutputWorkingTerm( pixCoord, 0.67 );
				}

				// Snap to pixel centers, XeGTAO.esh:481.
				sampleOffset = round( sampleOffset ) * consts.ViewportPixelSize;

				vec2 sampleScreenPos0 = normalisedScreenPos + sampleOffset;

				float SZ0 = ( 
					textureLod( sampler2D( u_InDepthGTAO, s_PointSampler ),  sampleScreenPos0, mipLevel ).x );
				
				vec3 samplePos0 = XeGTAO_ComputeViewspacePosition( sampleScreenPos0, SZ0, consts );

				vec2 sampleScreenPos1 = normalisedScreenPos - sampleOffset;
				
				float SZ1 = (
					textureLod( sampler2D( u_InDepthGTAO, s_PointSampler ), sampleScreenPos1, mipLevel ).x );
				
				vec3 samplePos1 = XeGTAO_ComputeViewspacePosition( sampleScreenPos1, SZ1, consts );

				vec3 sampleDelta0 = samplePos0 - pixCentrePos;
				vec3 sampleDelta1 = samplePos1 - pixCentrePos;
				float sampleDist0 = length( sampleDelta0 );
				float sampleDist1 = length( sampleDelta1 );

				if (sampleDist0 < 0.0001 || sampleDist1 < 0.0001)
					continue;

				vec3 sampleHorizonVec0 = sampleDelta0 / sampleDist0;
				vec3 sampleHorizonVec1 = sampleDelta1 / sampleDist1;

				float falloffBase0 =
					length( vec3( sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * ( 1.0 + thinOccluderCompensation ) ) );
				float falloffBase1 =
					length( vec3( sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * ( 1.0 + thinOccluderCompensation ) ) );
				float weight0 = clamp( falloffBase0 * falloffMul + falloffAdd, 0.0, 1.0 );
				float weight1 = clamp( falloffBase1 * falloffMul + falloffAdd, 0.0, 1.0 );

				float shc0 = dot( sampleHorizonVec0, viewVec );
				float shc1 = dot( sampleHorizonVec1, viewVec );

				shc0 = mix( lowHorizonCos0, shc0, weight0 );
				shc1 = mix( lowHorizonCos1, shc1, weight1 );

				horizonCos0 = max( horizonCos0, shc0 );
				horizonCos1 = max( horizonCos1, shc1 );
			}

			projectedNormalVecLength = mix( projectedNormalVecLength, 1.0, 0.05 );

			float h0 = -XeGTAO_FastACos( horizonCos1 );
			float h1 = XeGTAO_FastACos( horizonCos0 );
			float iarc0 = ( cosNorm + 2.0 * h0 * sin( n ) - cos( 2.0 * h0 - n ) ) * 0.25;
			float iarc1 = ( cosNorm + 2.0 * h1 * sin( n ) - cos( 2.0 * h1 - n ) ) * 0.25;
			float localVisibility = projectedNormalVecLength * ( iarc0 + iarc1 );
			visibility += localVisibility;
		}

		visibility /= sliceCount;
		visibility = pow( max( 0.0, visibility ), consts.FinalValuePower );
		visibility = max( 0.03, visibility );
	}


	return XeGTAO_OutputWorkingTerm( pixCoord, visibility );
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() 
{
	// Do not write if out of bounds.
	ivec2 pixCoord = ivec2( gl_GlobalInvocationID.xy );
	if( any( greaterThanEqual( pixCoord, u_ExtraData.ViewportSize ) ) ) 
	{
		return;
	}

	GTAOConsts consts;
	consts.ViewportPixelSize = u_ExtraData.ViewportPixelSize;
	consts.NDCToViewMul = u_ExtraData.NDCToViewMul;
	consts.NDCToViewAdd = u_ExtraData.NDCToViewAdd;
	consts.NDCToViewMul_x_PixelSize = pc_Params.NDCToViewMul_x_PixelSize;
	consts.EffectRadius = pc_Params.EffectRadius;
	consts.EffectFalloffRange = pc_Params.EffectFalloffRange;
	consts.RadiusMultiplier = pc_Params.RadiusMultiplier;
	consts.FinalValuePower = pc_Params.FinalValuePower;
	consts.SampleDistributionPower = pc_Params.SampleDistributionPower;
	consts.ThinOccluderCompensation = pc_Params.ThinOccluderCompensation;
	consts.DepthMIPSamplingOffset = pc_Params.DepthMipSamplingOffset;
	consts.SliceCount = pc_Params.SliceCount;
	consts.StepsPerSlice = pc_Params.StepsPerSlice;

	vec3 viewspaceNormal = XeGTAO_ComputeViewspaceNormal( pixCoord, consts );
	
	ivec2 vmax = u_ExtraData.ViewportSize - 1;

	float centreZ = XeGTAO_ScreenSpaceToViewSpaceDepth( 
		texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), 
		clamp( pixCoord, ivec2( 0 ), vmax ), 0 ).r, consts 
	);

	float leftZ = XeGTAO_ScreenSpaceToViewSpaceDepth( 
		texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), 
		clamp( pixCoord + ivec2( -1.0, 0.0 ), ivec2( 0 ), vmax ), 0 ).r, consts 
	);

	float rightZ = XeGTAO_ScreenSpaceToViewSpaceDepth( 
		texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), 
		clamp( pixCoord + ivec2( 1.0, 0.0 ), ivec2( 0 ), vmax ), 0 ).r, consts 
	);

	float topZ = XeGTAO_ScreenSpaceToViewSpaceDepth( 
		texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), 
		clamp( pixCoord + ivec2( 0.0, -1.0 ), ivec2( 0 ), vmax ), 0 ).r, consts 
	);

	float bottomZ = XeGTAO_ScreenSpaceToViewSpaceDepth( 
		texelFetch( sampler2D( u_InDepthPreDepth, s_PointSampler ), 
		clamp( pixCoord + ivec2( 0.0, 1.0 ), ivec2( 0 ), vmax ), 0 ).r, consts 
	);

	vec4 edgesLRTB = XeGTAO_CalculateEdges( centreZ, leftZ, rightZ, topZ, bottomZ );

	vec2 noise = SpatioTemporalNoise( uvec2( pixCoord ), 0 );

	XeGTAO_MainPass( pixCoord, noise, viewspaceNormal, centreZ, edgesLRTB, consts );
}
