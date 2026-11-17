//
// Volumetric Clouds, Temporal reconstruction
//

#type compute
#version 450 core

// Match CloudsRender.glsl.
#define CLOUDS_REVERSED_Z 0
#define CLOUDS_FLIP_Y 0

layout(set = 0, binding = 0) uniform sampler2D u_CloudsCurrent;
layout(set = 0, binding = 1) uniform sampler2D u_CloudsDistance;
layout(set = 0, binding = 2) uniform sampler2D u_CloudsHistory;

layout(set = 0, binding = 3, rgba16f) uniform writeonly image2D o_CloudsOut;

layout(set = 0, binding = 4) uniform ReconstructData
{
	mat4  InverseViewProj;      // Current frame
	mat4  PrevViewProj;         // Previous frame
	vec4  CameraPosMeters;      // Current camera position in metres

	int   TraceOffsetX;
	int   TraceOffsetY;
	int   HistoryValid;
	float BlendTraced;          // weight of the new sample on the pixel traced this frame
	float BlendOther;           // weight of the neighbouring sample on the other 3 pixels

	float _Pad0;
	float _Pad1;
	float _Pad2;
} u_Data;

// TODO: Rethink this.
vec4 SampleBilinear( sampler2D tex, vec2 uv )
{
	ivec2 size = textureSize( tex, 0 );
	vec2 p = uv * vec2( size ) - 0.5;
	ivec2 i = ivec2( floor( p ) );
	vec2 f = p - vec2( i );

	ivec2 maxP = size - 1;
	vec4 a = texelFetch( tex, clamp( i,                ivec2( 0 ), maxP ), 0 );
	vec4 b = texelFetch( tex, clamp( i + ivec2( 1, 0 ), ivec2( 0 ), maxP ), 0 );
	vec4 c = texelFetch( tex, clamp( i + ivec2( 0, 1 ), ivec2( 0 ), maxP ), 0 );
	vec4 d = texelFetch( tex, clamp( i + ivec2( 1, 1 ), ivec2( 0 ), maxP ), 0 );

	return mix( mix( a, b, f.x ), mix( c, d, f.x ), f.y );
}

vec3 ViewDirection( vec2 uv )
{
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

	vec4 nearClip = u_Data.InverseViewProj * vec4( ndc, kNearDepth, 1.0 );
	vec4 farClip  = u_Data.InverseViewProj * vec4( ndc, 1.0 - kNearDepth, 1.0 );

	return normalize( farClip.xyz / farClip.w - nearClip.xyz / nearClip.w );
}

layout( local_size_x = 8, local_size_y = 8 ) in;
void main()
{
	ivec2 px = ivec2( gl_GlobalInvocationID.xy );
	ivec2 size = imageSize( o_CloudsOut );

	if( px.x >= size.x || px.y >= size.y )
		return;

	ivec2 traceSize = textureSize( u_CloudsCurrent, 0 );
	ivec2 traceMax = traceSize - 1;
	ivec2 traceOffset = ivec2( u_Data.TraceOffsetX, u_Data.TraceOffsetY );

	vec2 tracePos = ( vec2( px ) - vec2( traceOffset ) ) * 0.5;
	ivec2 i0 = ivec2( floor( tracePos ) );
	vec2 f = tracePos - vec2( i0 );

	ivec2 p00 = clamp( i0,                ivec2( 0 ), traceMax );
	ivec2 p10 = clamp( i0 + ivec2( 1, 0 ), ivec2( 0 ), traceMax );
	ivec2 p01 = clamp( i0 + ivec2( 0, 1 ), ivec2( 0 ), traceMax );
	ivec2 p11 = clamp( i0 + ivec2( 1, 1 ), ivec2( 0 ), traceMax );

	vec4 c00 = texelFetch( u_CloudsCurrent, p00, 0 );
	vec4 c10 = texelFetch( u_CloudsCurrent, p10, 0 );
	vec4 c01 = texelFetch( u_CloudsCurrent, p01, 0 );
	vec4 c11 = texelFetch( u_CloudsCurrent, p11, 0 );

	// f is exactly 0 on the pixel traced this frame, so it gets its own sample.
	bool traced = all( equal( f, vec2( 0.0 ) ) );
	vec4 current = mix( mix( c00, c10, f.x ), mix( c01, c11, f.x ), f.y );

	// Neighbourhood of the newest samples around this position, used to reject stale history.
	ivec2 centre = clamp( i0 + ivec2( greaterThanEqual( f, vec2( 0.5 ) ) ), ivec2( 0 ), traceMax );

	vec4 boxMin = current;
	vec4 boxMax = current;

	for( int y = -1; y <= 1; ++y )
	{
		for( int x = -1; x <= 1; ++x )
		{
			vec4 s = texelFetch( u_CloudsCurrent, clamp( centre + ivec2( x, y ), ivec2( 0 ), traceMax ), 0 );
			boxMin = min( boxMin, s );
			boxMax = max( boxMax, s );
		}
	}

	vec2 uv = ( vec2( px ) + 0.5 ) / vec2( size );
	vec3 dir = ViewDirection( uv );
	float distanceKm = min(
		min( texelFetch( u_CloudsDistance, p00, 0 ).r, texelFetch( u_CloudsDistance, p10, 0 ).r ),
		min( texelFetch( u_CloudsDistance, p01, 0 ).r, texelFetch( u_CloudsDistance, p11, 0 ).r ) );

	vec3 worldPos = u_Data.CameraPosMeters.xyz + dir * distanceKm * 1000.0;
	vec4 prevClip = u_Data.PrevViewProj * vec4( worldPos, 1.0 );

	bool historyValid = u_Data.HistoryValid != 0 && prevClip.w > 0.0;

	vec2 prevUv = vec2( 0.0 );
	if( historyValid )
	{
		vec2 prevNdc = prevClip.xy / prevClip.w;
#if CLOUDS_FLIP_Y
		prevNdc.y = -prevNdc.y;
#endif
		prevUv = prevNdc * 0.5 + 0.5;

		historyValid = all( greaterThanEqual( prevUv, vec2( 0.0 ) ) ) && all( lessThanEqual( prevUv, vec2( 1.0 ) ) );
	}

	vec4 result;

	if( !historyValid )
	{
		result = current;
	}
	else
	{
		vec4 history = SampleBilinear( u_CloudsHistory, prevUv );
		history = clamp( history, boxMin, boxMax );

		float blend = traced ? u_Data.BlendTraced : u_Data.BlendOther;
		result = mix( history, current, blend );
	}

	if( any( isnan( result ) ) || any( isinf( result ) ) )
		result = current;

	imageStore( o_CloudsOut, px, result );
}
