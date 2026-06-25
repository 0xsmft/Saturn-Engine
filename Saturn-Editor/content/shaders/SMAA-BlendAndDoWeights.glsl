// Subpixel Morphological Anti-Aliasing (Blending and weights stage + final composition)
//
// Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
// Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
// Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
// Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
// Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
//
// The source of this shader can be found at:
// https://github.com/iryoku/smaa/blob/master/SMAA.hlsl

#type compute
#version 460

layout(push_constant) uniform Params
{
	// SMAA_RT_METRICS
	vec4 Metrics;

	// SMAA_THRESHOLD
	float Threshold;

	// SMAA_CORNER_ROUNDING_NORMAL
	float CornerRoundingNorm;

	// SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR
	float LocalContrastAdaptation;
} pc_Params;

// Outputs
// I don't even know what restrict does.
layout(set = 0, binding = 0, rgba16f) restrict writeonly uniform image2D o_Output;

layout(set = 0, binding = 1) uniform texture2D u_InputColorTexture;
layout(set = 0, binding = 2) uniform texture2D u_EdgesTexture;
layout(set = 0, binding = 3) uniform texture2D u_SearchTexture; // SMAA_SearchTex.tga
layout(set = 0, binding = 4) uniform texture2D u_AreaTexture;	// SMAA_AreaTex.tga
layout(set = 0, binding = 5) uniform sampler   s_LinearSampler;

shared uint s_BlendWeights[ 33 ][ 33 ];

vec4 SampleLinearZero( texture2D texture, vec2 coord ) 
{
	return textureLod( sampler2D( texture, s_LinearSampler ), coord, 0.0 );
}

vec2 SampleEdge( vec2 uv )
{
	return SampleLinearZero( u_EdgesTexture, uv ).xy;
}

vec2 SampleEdgeOffset( vec2 uv, vec2 off )
{
    vec2 param = uv + ( off * pc_Params.Metrics.xy );
    return SampleLinearZero( u_EdgesTexture, param ).xy;
}

void SMAAMovc( bvec2 cond, inout vec2 variable, vec2 value )
{
	if( cond.x ) variable.x = value.x;
	if( cond.y ) variable.y = value.y;
}

void SMAAMovc( bvec4 cond, inout vec4 variable, vec4 value )
{
	SMAAMovc( cond.xy, variable.xy, value.xy );
	SMAAMovc( cond.zw, variable.zw, value.zw );
}

float SMAASearchLength( vec2 e, float offset )
{
	vec2 scale = vec2( 66.0, 33.0 ) * vec2( 0.5, -1.0 );
	vec2 bias = vec2( 66.0, 33.0 ) * vec2( offset, 1.0 );

	scale += vec2( -1.0,  1.0 );
	bias  += vec2(  0.5, -0.5 );

	scale *= 1.0 / vec2( 64.0, 16.0 );
	bias  *= 1.0 / vec2( 64.0, 16.0 );

	return SampleLinearZero( u_SearchTexture, fma( scale, bias, e ) ).x;
}

vec2 SMAADecodeDiagBilinearAccess( vec2 e )
{
	// Bilinear access for fetching 'e' have a 0.25 offset, and we are
	// interested in the R and G edges:
	//
	// +---G---+-------+
	// |   x o R   x   |
	// +-------+-------+
	//
	// Then, if one of these edge is enabled:
	//   Red:   (0.75 * X + 0.25 * 1) => 0.25 or 1.0
	//   Green: (0.75 * 1 + 0.25 * X) => 0.75 or 1.0
	//
	// This function will unpack the values (mad + mul + round):
	// wolframalpha.com: round(x * abs(5 * x - 5 * 0.75)) plot 0 to 1
	e.r = e.r * abs(5.0 * e.r - 5.0 * 0.75);
	return round(e);
}

vec4 SMAADecodeDiagBilinearAccess( vec4 e ) 
{
	e.rb = e.rb * abs( 5.0 * e.rb - 5.0 * 0.75 );
	return round( e );
}

float SMAASearchXLeft( vec2 texCoord, float end )
{
	vec2 e = vec2( 0.0, 1.0 );

	while( texCoord.x > end && e.g > 0.8281 && e.r == 0.0 )
	{
		e = SampleEdge( texCoord ).rg;
		texCoord = fma( vec2( -2.0, -0.0 ), pc_Params.Metrics.xy, texCoord );
	}

	float off = fma( -( 255.0 / 127.0 ), SMAASearchLength( e, 0.0 ), 3.25 );
	return fma( pc_Params.Metrics.x, off, texCoord.x );
}

float SMAASearchXRight( vec2 texCoord, float end )
{
	vec2 e = vec2( 0.0, 1.0 );

	while( texCoord.x < end && e.g > 0.8281 && e.r == 0.0 )
	{
		e = SampleEdge( texCoord ).rg;
		texCoord = fma( vec2( 2.0, 0.0 ), pc_Params.Metrics.xy, texCoord );
	}

	float offset = fma( -( 255.0 / 127.0 ), SMAASearchLength( e, 0.5 ), 3.25 );
	return fma( -pc_Params.Metrics.x, offset, texCoord.x );
}

float SMAASearchYUp( vec2 texCoord, float end )
{
	vec2 e = vec2( 1.0, 0.0 );

	while( texCoord.y > end && e.r > 0.8281 && e.g == 0.0 )
	{
		e = SampleEdge( texCoord ).rg;
		texCoord = fma( vec2( -0.0, -2.0 ), pc_Params.Metrics.xy, texCoord );
	}

	float offset = fma( -( 255.0 / 127.0 ), SMAASearchLength( e.gr, 0.0 ), 3.25 );
	return fma( pc_Params.Metrics.y, offset, texCoord.y );
}

float SMAASearchYDown( vec2 texCoord, float end )
{
	vec2 e = vec2( 1.0, 0.0 );

	while( texCoord.y < end && e.r > 0.8281 && e.g == 0.0 )
	{
		e = SampleEdge( texCoord ).rg;
		texCoord = fma( vec2( 0.0, 2.0 ), pc_Params.Metrics.xy, texCoord );
	}

	float offset = fma( -( 255.0 / 127.0 ), SMAASearchLength( e.gr, 0.5 ), 3.25 );
	return fma( -pc_Params.Metrics.y, offset, texCoord.y );
}

#define SMAA_MAX_SEARCH_STEPS_DIAG 8

vec2 SMAASearchDiag1( vec2 texCoord, vec2 dir, out vec2 e )
{
	vec4 coord = vec4( texCoord, -1.0, 1.0 );
	vec3 t = vec3( pc_Params.Metrics.xy, 1.0 );

	while( coord.z < float( SMAA_MAX_SEARCH_STEPS_DIAG - 1 ) && coord.w > 0.9 ) 
	{
		coord.xyz = fma( t, vec3( dir, 1.0 ), coord.xyz );
		e = SampleEdge( coord.xy ).rg;
		coord.w = dot( e, vec2( 0.5, 0.5 ) );
	}

	return coord.zw;
}

vec2 SMAASearchDiag2( vec2 texCoord, vec2 dir, out vec2 e )
{
	vec4 coord = vec4( texCoord, -1.0, 1.0 );
	coord.x += 0.25 * pc_Params.Metrics.x;
	vec3 t = vec3( pc_Params.Metrics.xy, 1.0 );

	while( coord.z < float( SMAA_MAX_SEARCH_STEPS_DIAG - 1 ) && coord.w > 0.9 )
	{
		coord.xyz = fma( t, vec3( dir, 1.0 ), coord.xyz );

		e = SampleEdge( coord.xy ).rg;
		e = SMAADecodeDiagBilinearAccess( e );

		coord.w = dot( e, vec2( 0.5, 0.5 ) );
	}

	return coord.zw;
}

#define SMAA_AREATEX_PIXEL_SIZE 1.0 / vec2( 160, 560 )
#define SMAA_AREATEX_SUBTEX_SIZE 1.0 / 7

vec2 SMAAAreaDiag( vec2 dist, vec2 e, float offset )
{
	vec2 texCoord = fma( vec2( 20, 20 ), e, dist );

	texCoord = fma( SMAA_AREATEX_PIXEL_SIZE, texCoord, 0.5 * SMAA_AREATEX_PIXEL_SIZE );
	texCoord.x += 0.5;
	texCoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

	return SampleLinearZero( u_AreaTexture, texCoord ).xy;
}

vec2 SMAAArea( vec2 dist, float e1, float e2, float offset )
{
	vec2 texCoord = fma( vec2( 16, 16 ), round( 4.0 * vec2( e1, e2 ) ), dist );

	texCoord = fma( SMAA_AREATEX_PIXEL_SIZE, texCoord, 0.5 * SMAA_AREATEX_PIXEL_SIZE );
	texCoord.y = fma( SMAA_AREATEX_SUBTEX_SIZE, offset, texCoord.y );

	return SampleLinearZero( u_AreaTexture, texCoord ).xy;
}

vec2 SMAACalculateDiagWeights( vec2 texCoord, vec2 e, vec4 subSampleIndices ) 
{
	vec2 weights = vec2( 0.0, 0.0 );

	vec4 d;
	vec2 end;

	if( e.r > 0.0 ) 
	{
		d.xz = SMAASearchDiag1( texCoord, vec2( -1.0, 1.0 ), end );
		d.x += float( end.y > 0.9 );
	}
	else
	{
		d.xz = vec2( 0.0 );
	}

	d.yw = SMAASearchDiag1( texCoord, vec2( 1.0, -1.0 ), end );

	if( ( d.x + d.y ) > 2.0 ) 
	{
		// Fetch the corssing edges:
		vec4 coords = fma( vec4( -d.x + 0.25, d.x, d.y, -d.y - 0.25 ), pc_Params.Metrics.xyxy, texCoord.xyxy );
		vec4 c;
		c.xy   = SampleEdgeOffset( coords.xy, vec2( -1.0, 0.0 ) ).rg;
		c.zw   = SampleEdgeOffset( coords.zw, vec2(  1.0, 0.0 ) ).rg;
		c.yxwz = SMAADecodeDiagBilinearAccess( c.yxwz );

		// Merge crossing edges at each side into a single value:
		vec2 cc = fma( vec2( 2.0, 2.0 ), c.xz, c.yw );

		// Remove the crossing edge if we didn't find the end of the line:
		SMAAMovc( bvec2( step( 0.9, d.zw ) ), cc, vec2( 0.0, 0.0 ) );

		// Fetch the areas for this line:
		weights += SMAAAreaDiag( d.xy, cc, subSampleIndices.z );
	}

	// Search for the line ends:

	d.xy = SMAASearchDiag2( texCoord, vec2( -1.0, -1.0 ), end );

	if( SampleEdgeOffset( texCoord, vec2( 1.0, 0.0 ) ).x > 0.0 ) 
	{
		d.yw = SMAASearchDiag2( texCoord, vec2( 1.0 ), end );
		d.y += float( end.y > 0.9 );
	}
	else
	{
		d.yw = vec2( 0.0 );
	}

	if( ( d.x + d.y ) > 2.0 )
	{
		vec4 coords = fma( vec4( -d.x, -d.y, d.y, d.y ), pc_Params.Metrics.xyxy, texCoord.xyxy );
		vec4 c;
		c.x   = SampleEdgeOffset( coords.xy, ivec2( -1,  0 ) ).g;
		c.y   = SampleEdgeOffset( coords.xy, ivec2(  0, -1 ) ).r;
		c.zw  = SampleEdgeOffset( coords.zw, ivec2(  1,  0 ) ).gr;

		vec2 cc = fma( vec2( 2.0 ), c.xz, c.yw );

		SMAAMovc( bvec2( step( 0.9, d.zw ) ), cc, vec2( 0.0 ) );

		weights += SMAAAreaDiag( d.xy, cc, subSampleIndices.z );
	}

	return weights;
}

void SMAADetectHorizontalCornerPattern( inout vec2 weights, vec4 texCoord, vec2 d )
{
	vec2 leftRight = step( d.xy, d.yx );
	vec2 rounding = ( 1.0 - pc_Params.CornerRoundingNorm ) * leftRight;

	rounding /= leftRight.x + leftRight.y;

	vec2 factor = vec2( 1.0, 1.0 );
	factor.x -= rounding.x * SampleEdgeOffset( texCoord.xy, vec2( 0.0, 1.0 ) ).r;
	factor.x -= rounding.y * SampleEdgeOffset( texCoord.zw, vec2( 1.0, 1.0 ) ).r;

	factor.y -= rounding.x * SampleEdgeOffset( texCoord.xy, vec2( 0.0, -2.0 ) ).r;
	factor.y -= rounding.y * SampleEdgeOffset( texCoord.zw, vec2( 1.0, -2.0 ) ).r;

	weights *= clamp( factor, vec2( 0.0 ), vec2( 1.0 ) );
}

void SMAADetectVerticalCornerPattern( inout vec2 weights, vec4 texCoord, vec2 d )
{
	vec2 leftRight = step( d.xy, d.yx );
	vec2 rounding = ( 1.0 - pc_Params.CornerRoundingNorm ) * leftRight;

	rounding /= leftRight.x + leftRight.y;

	vec2 factor = vec2( 1.0, 1.0 );
	factor.x -= rounding.x * SampleEdgeOffset( texCoord.xy, vec2( 1.0, 0.0 ) ).g;
	factor.x -= rounding.y * SampleEdgeOffset( texCoord.zw, vec2( 1.0, 1.0 ) ).g;

	factor.y -= rounding.x * SampleEdgeOffset( texCoord.xy, vec2( -2.0,  0.0 ) ).g;
	factor.y -= rounding.y * SampleEdgeOffset( texCoord.zw, vec2( -2.0,  1.0 ) ).g;

	weights *= clamp( factor, vec2( 0.0 ), vec2( 1.0 ) );
}

// ~ SMAABlendingWeightCalculationPS - SMAA.hlsl
vec4 SMAABlendingWeightCalculation( ivec2 pixelCoord )
{
	vec4 weights = vec4(0.0);
	vec2 texCoord = ( vec2( pixelCoord ) + vec2( 0.5 ) ) * pc_Params.Metrics.xy;
	vec2 xpixelCoord = texCoord * pc_Params.Metrics.zw;

	vec4 offset0 = fma( pc_Params.Metrics.xyxy, vec4( -0.25, -0.125, 1.25, -0.125 ), texCoord.xyxy );
	vec4 offset1 = fma( pc_Params.Metrics.xyxy, vec4( -0.125, -0.25, -0.125, 1.25 ), texCoord.xyxy );
	vec4 offset2 = fma( pc_Params.Metrics.xyxy, vec4( -32.0, 32.0, -32.0, 32.0 ), vec4( offset0.xz, offset1.yw ) );

	vec4 subSampleIndices = vec4( 0.0 );
	vec2 e = SampleEdge( texCoord );

	// Edge at north
	if( e.g > 0.0 )
	{
		weights.rg = SMAACalculateDiagWeights( texCoord, e, subSampleIndices );
		if( weights.r == ( -weights.g ) ) 
		{
			vec2 d;
			vec3 coords;

			coords.x = SMAASearchXLeft( offset0.xy, offset2.x );
			coords.y = offset1.y;
			d.x = coords.x;

			float e1 = SampleEdge( coords.xy ).r;

			coords.z = SMAASearchXRight( offset0.zw, offset2.y );
			d.y = coords.z;

			d = abs( round( fma( pc_Params.Metrics.zz, d, -xpixelCoord.xx ) ) );

			vec2 sqrt_d = sqrt( d );

			float e2 = SampleEdgeOffset( coords.zy, vec2( 1.0, 0.0 ) ).r;

			weights.rg = SMAAArea( sqrt_d, e1, e2, subSampleIndices.y );

			coords.y = texCoord.y;

			SMAADetectHorizontalCornerPattern( weights.rg, coords.xyzy, d );
		}
		else
		{
			e.x = 0.0;
		}
	}

	// Edge at west
	if( e.r > 0.0 ) 
	{
		vec2 d;
		vec3 coords;
		coords.x = SMAASearchYUp( offset1.xy, offset2.z );
		coords.y = offset0.x;

		d.x = coords.y;

		float e1 = SampleEdge( coords.xy ).g;

		coords.z = SMAASearchYDown( offset1.zw, offset2.w );
		d.y = coords.z;

		d = abs( round( fma( pc_Params.Metrics.ww, d, -xpixelCoord.yy ) ) );

		vec2 sqrt_d = sqrt( d );

		float e2 = SampleEdgeOffset( coords.zy, vec2( 0.0, 1.0 ) ).g;

		weights.zw = SMAAArea( sqrt_d, e1, e2, subSampleIndices.x );

		coords.x = texCoord.x;
		SMAADetectVerticalCornerPattern( weights.zw, coords.xyzy, d );
	}

	return weights;
}

void StoreWeights( uint y, uint x, vec4 weight ) 
{
	s_BlendWeights[ y ][ x ] = packUnorm4x8( weight );
}

vec4 LoadWeights( uint y, uint x )
{
	return unpackUnorm4x8( s_BlendWeights[ y ][ x ] );
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
	ivec2 size = ivec2( pc_Params.Metrics.zw );
	uint localIdx = gl_LocalInvocationIndex;
	uvec2 localID = gl_LocalInvocationID.xy;
	ivec2 groupBase = ivec2( gl_WorkGroupID.xy ) * ivec2( 32 );

	ivec2 myPixel = groupBase + ivec2( localID );
	
	vec4 weights = SMAABlendingWeightCalculation( myPixel );
	StoreWeights( localID.y, localID.x, weights );

	if( localIdx < 65u ) 
	{
		uint sx;
		uint sy;

		if( localIdx < 32u ) 
		{
			sx = 32u;
			sy = localIdx;
		}
		else
		{
			sx = localIdx - 32u;
			sy = 32u;
		}

		ivec2 borderPixel = groupBase + ivec2( int( sx ), int( sy ) );
		StoreWeights( sy, sx, SMAABlendingWeightCalculation( borderPixel ) );
	}

	barrier();

	ivec2 pixelCoord = groupBase + ivec2( localID );
	if( pixelCoord.x >= size.x || pixelCoord.y >= size.y ) 
	{
		return;
	}

	vec2 texCoord = ( vec2( pixelCoord ) + vec2( 0.5 ) ) * pc_Params.Metrics.xy;
	vec4 loadedWeights = LoadWeights( localID.y, localID.x );

	vec4 a;
	a.w = loadedWeights.xz.x;
	a.z = loadedWeights.xz.y;
	a.x = LoadWeights( localID.y, localID.x + 1u ).w;
	a.y = LoadWeights( localID.y + 1u, localID.x ).y;

	if( dot( a, vec4( 1.0 ) ) < 1e-5 ) 
	{
		vec4 color = SampleLinearZero( u_InputColorTexture, texCoord );
		imageStore( o_Output, pixelCoord, color );
		
		return;
	}

	// max(horizontal) > max(vertical)
	bool h = max( a.x, a.z ) > max( a.y, a.w );

	vec4 blendingOffset = vec4( 0.0, a.y, 0.0, a.w );
	vec2 blendingWeight = a.yw;

	SMAAMovc( bvec4( h ), blendingOffset, vec4( a.x, 0.0, a.z, 0.0 ) );
	SMAAMovc( bvec2( h ), blendingWeight, a.xz );
	blendingWeight /= dot( blendingWeight, vec2( 1.0 ) );

	vec4 blendingCoord = fma( blendingOffset, vec4( pc_Params.Metrics.xy, -pc_Params.Metrics.xy ), texCoord.xyxy );
	vec4 color = blendingWeight.x * SampleLinearZero( u_InputColorTexture, blendingCoord.xy );
	color += blendingWeight.y * SampleLinearZero( u_InputColorTexture, blendingCoord.zw );

	imageStore( o_Output, pixelCoord, color );
}
