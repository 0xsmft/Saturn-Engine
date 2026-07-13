// GTAO (Ground-Truth Ambient Occlusion) Prefilter
//
// This implementation is based from the following items:
//
// - Erin Catto, box3d (https://github.com/erincatto/box3d), MIT
// - Bobby Anguelov, Esoterica Engine (https://github.com/BobbyAnguelov/Esoterica), MIT
// - Intel Corporation, XeGTAO (https://github.com/GameTechDev/XeGTAO), MIT
//

#type compute
#version 460

layout(set = 0, binding = 0) uniform texture2D u_InDepth;
layout(set = 0, binding = 1) uniform sampler   s_LinearSampler;

layout(set = 0, binding = 2, r32f) uniform writeonly image2D o_OutDepths0;
layout(set = 0, binding = 3, r32f) uniform writeonly image2D o_OutDepths1;
layout(set = 0, binding = 4, r32f) uniform writeonly image2D o_OutDepths2;
layout(set = 0, binding = 5, r32f) uniform writeonly image2D o_OutDepths3;
layout(set = 0, binding = 6, r32f) uniform writeonly image2D o_OutDepths4;

layout(push_constant) uniform Params
{
	vec2 DepthUnpackConsts;
	vec2 ViewportPixelSize;
	float EffectRadius;
	float EffectFalloffRange;
	float RadiusMultiplier;
} pc_Params;

shared float g_ScratchDepths[ 8 ][ 8 ];

float XeGTAO_ClampDepth( float depth )
{
	return clamp( depth, 0.0, 3.402823466e+38 );
}

// XeGTAO_DepthMIPFilter, weighted average that biases toward the
// closest tap (smallest depth) and suppresses contributions from taps
// outside an effect-radius-scaled falloff window. 
float XeGTAO_DepthMIPFilter( float depth0, float depth1, float depth2, float depth3 )
{
	float maxDepth = max( max( depth0, depth1 ), max( depth2, depth3 ) );

	const float depthRangeScaleFactor = 0.75;
	float effectRadius = depthRangeScaleFactor * pc_Params.EffectRadius * pc_Params.RadiusMultiplier;
	float falloffRange = pc_Params.EffectFalloffRange * effectRadius;
	float falloffFrom  = effectRadius * ( 1.0 - pc_Params.EffectFalloffRange );
	float falloffMul = -1.0 / falloffRange;
	float falloffAdd = falloffFrom / falloffRange + 1.0;

	float w0 = clamp( ( maxDepth - depth0 ) * falloffMul + falloffAdd, 0.0, 1.0 );
	float w1 = clamp( ( maxDepth - depth1 ) * falloffMul + falloffAdd, 0.0, 1.0 );
	float w2 = clamp( ( maxDepth - depth2 ) * falloffMul + falloffAdd, 0.0, 1.0 );
	float w3 = clamp( ( maxDepth - depth3 ) * falloffMul + falloffAdd, 0.0, 1.0 );

	float sumOfWeights = w0 + w1 + w2 + w3;
	return ( w0 * depth0 +
			 w1 * depth1 + 
			 w2 * depth2 + 
			 w3 * depth3 ) / sumOfWeights;
}

// Because the depth from our PreDepth pass will be linear 0-1, we need to convert that 
// to useful values from 0-far-clip, this allows us to gain better information on how far
// away objects are.
float ConvertPreDepthToViewSpace( float depth ) 
{
	return abs( pc_Params.DepthUnpackConsts.x / ( depth + pc_Params.DepthUnpackConsts.y ) );
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 dispatchThreadID = ivec2( gl_GlobalInvocationID.xy );
	ivec2 groupThreadID = ivec2( gl_LocalInvocationID.xy );
	ivec2 viewportMax = ivec2( pc_Params.ViewportPixelSize ) - 1;

	ivec2 pixCoord = dispatchThreadID * 2;
	ivec2 p0 = clamp( pixCoord + ivec2( 0, 0 ), ivec2( 0 ), viewportMax );
	ivec2 p1 = clamp( pixCoord + ivec2( 1, 0 ), ivec2( 0 ), viewportMax );
	ivec2 p2 = clamp( pixCoord + ivec2( 0, 1 ), ivec2( 0 ), viewportMax );
	ivec2 p3 = clamp( pixCoord + ivec2( 1, 1 ), ivec2( 0 ), viewportMax );

	// Covert hardware depth into view-space depth.
	vec4 screenDepth0 = texelFetch( sampler2D( u_InDepth, s_LinearSampler ), p0, 0 );
	vec4 screenDepth1 = texelFetch( sampler2D( u_InDepth, s_LinearSampler ), p1, 0 );
	vec4 screenDepth2 = texelFetch( sampler2D( u_InDepth, s_LinearSampler ), p2, 0 );
	vec4 screenDepth3 = texelFetch( sampler2D( u_InDepth, s_LinearSampler ), p3, 0 );

	float d0 = XeGTAO_ClampDepth( ConvertPreDepthToViewSpace( screenDepth0.r ) );
	float d1 = XeGTAO_ClampDepth( ConvertPreDepthToViewSpace( screenDepth1.r ) );
	float d2 = XeGTAO_ClampDepth( ConvertPreDepthToViewSpace( screenDepth2.r ) );
	float d3 = XeGTAO_ClampDepth( ConvertPreDepthToViewSpace( screenDepth3.r ) );

	if( all( lessThan( pixCoord + ivec2( 0, 0 ), ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
	{
		imageStore( o_OutDepths0, pixCoord + ivec2( 0, 0 ), vec4( d0, 0.0, 0.0, 0.0 ) );
	}

	if( all( lessThan( pixCoord + ivec2( 1, 0 ), ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
	{
		imageStore( o_OutDepths0, pixCoord + ivec2( 1, 0 ), vec4( d1, 0.0, 0.0, 0.0 ) );
	}
	
	if( all( lessThan( pixCoord + ivec2( 0, 1 ), ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
	{
		imageStore( o_OutDepths0, pixCoord + ivec2( 0, 1 ), vec4( d2, 0.0, 0.0, 0.0 ) );
	}

	if( all( lessThan( pixCoord + ivec2( 1, 1 ), ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
	{
		imageStore( o_OutDepths0, pixCoord + ivec2( 1, 1 ), vec4( d3, 0.0, 0.0, 0.0 ) );
	}

	// Every thread produces one mip-1 candidate (always stored in
	// scratch so the mip-2 reduction has a uniform view), conditionally
	// written to the mip-1 image only if dispatchThreadID is in bounds.
	float dm1 = XeGTAO_DepthMIPFilter( d0, d1, d2, d3 );
	if( all( lessThan( dispatchThreadID, ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
	{
		imageStore( o_OutDepths1, dispatchThreadID, vec4( dm1, 0.0, 0.0, 0.0 ) );
	}

	g_ScratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm1;

	barrier();

	float dm2 = 0.0;
	bool isMip2Producer = all( equal( groupThreadID & ivec2( 1 ), ivec2( 0 ) ) );
	if( isMip2Producer ) 
	{
		float inTL = g_ScratchDepths[ groupThreadID.x + 0 ][ groupThreadID.y + 0 ];
		float inTR = g_ScratchDepths[ groupThreadID.x + 1 ][ groupThreadID.y + 0 ];
		float inBL = g_ScratchDepths[ groupThreadID.x + 0 ][ groupThreadID.y + 1 ];
		float inBR = g_ScratchDepths[ groupThreadID.x + 1 ][ groupThreadID.y + 1 ];

		dm2 = XeGTAO_DepthMIPFilter( inTL, inTR, inBL, inBR );

		ivec2 dispatchID2 = ivec2( uvec2( dispatchThreadID ) >> 1u );
		if( all( lessThan( dispatchID2, ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
		{
			imageStore( o_OutDepths2, dispatchID2, vec4( dm2, 0.0, 0.0, 0.0 ) );
		}

		g_ScratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm2;
	}

	barrier();

	float dm3 = 0.0;
	bool isMip3Producer = all( equal( groupThreadID & ivec2( 3 ), ivec2( 0 ) ) );
	if ( isMip3Producer )
	{
		float inTL = g_ScratchDepths[ groupThreadID.x + 0 ][ groupThreadID.y + 0 ];
		float inTR = g_ScratchDepths[ groupThreadID.x + 2 ][ groupThreadID.y + 0 ];
		float inBL = g_ScratchDepths[ groupThreadID.x + 0 ][ groupThreadID.y + 2 ];
		float inBR = g_ScratchDepths[ groupThreadID.x + 2 ][ groupThreadID.y + 2 ];

		dm3 = XeGTAO_DepthMIPFilter( inTL, inTR, inBL, inBR );

		ivec2 dispatchID4 = ivec2( uvec2( dispatchThreadID ) >> 2u );
		if( all( lessThan( dispatchID4, ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
		{
			imageStore( o_OutDepths3, dispatchID4, vec4( dm3, 0.0, 0.0, 0.0 ) );
		}

		g_ScratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm3;
	}

	barrier();

	if( all( equal( groupThreadID, ivec2( 0 ) ) ) ) 
	{
		float inTL = g_ScratchDepths[ 0 ][ 0 ];
		float inTR = g_ScratchDepths[ 4 ][ 0 ];
		float inBL = g_ScratchDepths[ 0 ][ 4 ];
		float inBR = g_ScratchDepths[ 4 ][ 4 ];

		float dm4 = XeGTAO_DepthMIPFilter( inTL, inTR, inBL, inBR );

		ivec2 dispatchID8 = ivec2( uvec2( dispatchThreadID ) >> 3u );
		if( all( lessThan( dispatchID8, ivec2( pc_Params.ViewportPixelSize ) ) ) ) 
		{
			imageStore( o_OutDepths4, dispatchID8, vec4( dm4, 0.0, 0.0, 0.0 ) );
		}
	}
}
