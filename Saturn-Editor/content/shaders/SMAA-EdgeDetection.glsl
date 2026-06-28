// Subpixel Morphological Anti-Aliasing (Edge Detection stage)
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
#version 450 core

// Outputs
layout(binding = 0, rg8) restrict writeonly uniform image2D o_OutEdges;

// The final final color, even after the scene composite and gamma correction.
layout(binding = 1) uniform texture2D u_InFinalColor;

layout(binding = 2) uniform sampler s_PointSampler;

layout(push_constant) uniform Params 
{
	// SMAA_RT_METRICS
	vec4 Metrics;

	// SMAA_THRESHOLD
	float Threshold;

	// SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR
	float LocalContrastAdaptation;
} pc_Params;

vec4 SamplePoint( texture2D tex, vec2 coord )
{
    return textureLod( sampler2D( tex, s_PointSampler ), coord, 0.0 );
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() 
{
	ivec2 currentPixelCoord = ivec2( gl_GlobalInvocationID.xy );
	ivec2 size = ivec2( pc_Params.Metrics.zw );

	vec2 texCoord = vec2( currentPixelCoord + vec2( 0.5 ) ) * pc_Params.Metrics.xy;

	// Offsets based from: https://github.com/iryoku/smaa/blob/master/SMAA.hlsl#L647
	vec4 offset0 = ( pc_Params.Metrics.xyxy * vec4( -1.0, 0.0, 0.0, -1.0 ) ) + texCoord.xyxy;
	vec4 offset1 = ( pc_Params.Metrics.xyxy * vec4(  1.0, 0.0, 0.0,  1.0 ) ) + texCoord.xyxy;
	vec4 offset2 = ( pc_Params.Metrics.xyxy * vec4( -2.0, 0.0, 0.0, -2.0 ) ) + texCoord.xyxy;

	vec4 delta;
	vec3 centre = SamplePoint( u_InFinalColor, texCoord ).rgb;

	vec3 centreLeft = SamplePoint( u_InFinalColor, offset0.xy ).rgb;
	vec3 t = abs( centre - centreLeft );
	delta.x = max( max( t.r, t.g ), t.b );

	vec3 centreTop = SamplePoint( u_InFinalColor, offset0.zw ).rgb;
	t = abs( centre - centreTop );
	delta.y = max( max( t.r, t.g ), t.b );

	vec2 edges = step( pc_Params.Threshold, delta.xy );

	// Discard if there is no edge.
	if( dot( edges, vec2( 1.0, 1.0 ) ) == 0.0 ) 
	{
		imageStore( o_OutEdges, currentPixelCoord, vec4( 0.0 ) );
		return;
	}

	vec3 centreRight = SamplePoint( u_InFinalColor, offset1.xy ).rgb;
	t = abs( centre - centreRight );
	delta.z = max( max( t.r, t.g ), t.b );

	vec3 centreBottom = SamplePoint( u_InFinalColor, offset1.zw ).rgb;
	t = abs( centre - centreBottom );
	delta.w = max( max( t.r, t.g ), t.b );

	// Calculate the maximum delta in the direct neighbourhood.
	vec2 maxDelta = max( delta.xy, delta.zw );

	vec3 centreLeftLeft = SamplePoint( u_InFinalColor, offset2.xy ).rgb;
	t = abs( centre - centreLeftLeft );
	delta.z = max( max( t.r, t.g ), t.b  );

	vec3 centreTopTop = SamplePoint( u_InFinalColor, offset2.zw ).rgb;
	t = abs( centre - centreTopTop );
	delta.w = max( max( t.r, t.g ), t.b );

	maxDelta = max( maxDelta.xy, delta.zw );
	float finalDelta = max( maxDelta.x, maxDelta.y );

	edges.xy *= step( finalDelta, pc_Params.LocalContrastAdaptation * delta.xy );

	// Store the final pixel into the image.
	imageStore( o_OutEdges, currentPixelCoord, vec4( edges, 0.0, 0.0 ) );
}
