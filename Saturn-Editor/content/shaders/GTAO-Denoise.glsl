// GTAO (Ground-Truth Ambient Occlusion) Denoising

#type compute
#version 460

layout(set = 0, binding = 0) uniform texture2D u_GTAOIn;
layout(set = 0, binding = 1) uniform texture2D u_EdgesIn;
layout(set = 0, binding = 2) uniform sampler   s_LinearSampler;

layout(set = 0, binding = 3, r32f) uniform writeonly image2D o_GTAOOut;

const float XE_GTAO_OCCLUSION_TERM_SCALE = 1.5;

layout(push_constant) uniform u_Params
{
	ivec2 ViewportSize;
	float DenoiseBeta;
	uint HalfRes;
	uint FinalApply;
} pc_Params;

float FetchEdge( ivec2 coord )
{
	ivec2 vmax = pc_Params.ViewportSize - 1;
	ivec2 c = clamp( coord, ivec2( 0 ), vmax );
	return texelFetch( sampler2D( u_EdgesIn, s_LinearSampler ), c, 0 ).r;
}

float FetchAO( ivec2 coord )
{
	ivec2 vmax = pc_Params.ViewportSize - 1;
	ivec2 c = clamp( coord, ivec2( 0 ), vmax );
	return texelFetch( sampler2D( u_GTAOIn, s_LinearSampler ), c, 0 ).r;
}

void XeGTAO_AddSample( float ssaoValue, float edgeValue, inout float sum, inout float sumWeight )
{
	float weight = edgeValue;
	sum += weight * ssaoValue;
	sumWeight += weight;
}

void XeGTAO_Output( ivec2 pixCoord, float outputValue, bool finalApply )
{
	float scale = finalApply ? XE_GTAO_OCCLUSION_TERM_SCALE : 1.0;
	imageStore( o_GTAOOut, pixCoord, vec4( outputValue * scale, 0.0, 0.0, 0.0 ) );
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

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;
void main() 
{
	ivec2 dispatchThreadID = ivec2( gl_GlobalInvocationID.xy );

	ivec2 pixCoordBase = ivec2( dispatchThreadID.x * 2, dispatchThreadID.y );
	if( any( greaterThanEqual( pixCoordBase, pc_Params.ViewportSize ) ) ) 
	{
		return;
	}

	bool finalApply = ( pc_Params.FinalApply != 0 );
	float blurAmount = finalApply ? pc_Params.DenoiseBeta : ( pc_Params.DenoiseBeta / 5.0 );
	const float diagWeight = 0.85 * 0.5;

	float edgesN[ 4 ][ 3 ];
	float ao[ 4 ][ 3 ];

	for( int i = 0; i < 4; ++i ) 
	{
		for( int j = 0; j < 3; ++j ) 
		{
			ivec2 c = pixCoordBase + ivec2( i - 1, j - 1 );
			edgesN[ i ][ j ] = FetchEdge( c );
			ao[ i ][ j ] = FetchAO( c );
		}
	}

	for( int side = 0; side < 2; ++side )
	{
		ivec2 pixCoord = pixCoordBase + ivec2( side, 0 );
		if ( pixCoord.x >= pc_Params.ViewportSize.x )
		{
			break;
		}

		// Index helpers: column `c` in the 4-wide patch corresponds to
		// x = pixCoordBase.x + (c - 1). For side==0 the C column is 1
		// (the L/R neighbours sit at columns 0 / 2), for side==1 the C
		// column is 2 (neighbours at 1 / 3).
		int cC = 1 + side;
		int cL = cC - 1;
		int cR = cC + 1;

		vec4 edgesC_LRTB = XeGTAO_UnpackEdges( edgesN[ cC ][ 1 ] );
		vec4 edgesL_LRTB = XeGTAO_UnpackEdges( edgesN[ cL ][ 1 ] );
		vec4 edgesR_LRTB = XeGTAO_UnpackEdges( edgesN[ cR ][ 1 ] );
		vec4 edgesT_LRTB = XeGTAO_UnpackEdges( edgesN[ cC ][ 0 ] );
		vec4 edgesB_LRTB = XeGTAO_UnpackEdges( edgesN[ cC ][ 2 ] );

		edgesC_LRTB *= vec4( edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z );

		// Leak, XeGTAO.esh:839-843 (`#if 1` branch is the default and
		// only path Esoterica ships). Allows a small amount of bleed
		// when a pixel is surrounded by 3-4 edges, reducing aliasing.
		const float leakThreshold = 2.5;
		const float leakStrength = 0.5;
		float edginess =
			( clamp( 4.0 - leakThreshold - dot( edgesC_LRTB, vec4( 1.0 ) ), 0.0, 1.0 ) / ( 4.0 - leakThreshold ) ) *
			leakStrength;
		edgesC_LRTB = clamp( edgesC_LRTB + edginess, 0.0, 1.0 );

		// Diagonal weights.
		float weightTL = diagWeight * ( edgesC_LRTB.x * edgesL_LRTB.z + edgesC_LRTB.z * edgesT_LRTB.x );
		float weightTR = diagWeight * ( edgesC_LRTB.z * edgesT_LRTB.y + edgesC_LRTB.y * edgesR_LRTB.z );
		float weightBL = diagWeight * ( edgesC_LRTB.w * edgesB_LRTB.x + edgesC_LRTB.x * edgesL_LRTB.w );
		float weightBR = diagWeight * ( edgesC_LRTB.y * edgesR_LRTB.w + edgesC_LRTB.w * edgesB_LRTB.y );

		// AO neighbourhood for this side. Center at (cC, 1), L/R at
		// (cL, 1) / (cR, 1), T/B at (cC, 0) / (cC, 2), diagonals at the
		// 4 corners of the 3x3 around (cC, 1).
		float ssaoValue = ao[ cC ][ 1 ];
		float ssaoValueL = ao[ cL ][ 1 ];
		float ssaoValueR = ao[ cR ][ 1 ];
		float ssaoValueT = ao[ cC ][ 0 ];
		float ssaoValueB = ao[ cC ][ 2 ];
		float ssaoValueTL = ao[ cL ][ 0 ];
		float ssaoValueTR = ao[ cR ][ 0 ];
		float ssaoValueBL = ao[ cL ][ 2 ];
		float ssaoValueBR = ao[ cR ][ 2 ];

		float sumWeight = blurAmount;
		float sum = ssaoValue * sumWeight;

		XeGTAO_AddSample( ssaoValueL, edgesC_LRTB.x, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueR, edgesC_LRTB.y, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueT, edgesC_LRTB.z, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueB, edgesC_LRTB.w, sum, sumWeight );

		XeGTAO_AddSample( ssaoValueTL, weightTL, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueTR, weightTR, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueBL, weightBL, sum, sumWeight );
		XeGTAO_AddSample( ssaoValueBR, weightBR, sum, sumWeight );

		float aoTerm = sum / sumWeight;
		XeGTAO_Output( pixCoord, aoTerm, finalApply );

		imageStore( o_GTAOOut, pixCoord, vec4( aoTerm, 0, 0, 0 ) );
	}
}
