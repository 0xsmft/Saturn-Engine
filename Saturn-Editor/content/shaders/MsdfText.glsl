// Msdf Text Shader

#type vertex
#version 450

// Inputs
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_TexIndex;

layout(push_constant) uniform pc_ScalingAndTransfrom
{
	mat4 Projection;
} u_Transform; 

struct VertOut 
{
	vec2 TexCoord;
	vec4 Color;
};

layout( location = 0 ) out VertOut o_OutputData;
layout( location = 5 ) out flat float o_AtlasIndex;

void main() 
{
	o_OutputData.Color = a_Color;
	o_OutputData.TexCoord = a_TexCoord;
	o_AtlasIndex = a_TexIndex;

	gl_Position = u_Transform.Projection * vec4( a_Position, 0.0, 1.0 );
}

#type fragment
#version 450
//#extension GL_EXT_debug_printf : enable

struct VertOut 
{
	vec2 TexCoord;
	vec4 Color;
};

// In
layout( location = 0 ) in VertOut o_InputData;
layout( location = 5 ) in flat float o_AtlasIndex;

// Out
layout( location = 0 ) out vec4 FinalColor;

// Inputs
layout( set = 0, binding = 0 ) uniform sampler2D u_FontAtlases[16];

float Median( float r, float g, float b )
{
    return max( min( r, g ), min( max( r, g ), b ) );
}

float ScreenPxRange()
{
	float pxRange = 2.0f;
    vec2 unitRange = vec2( pxRange ) / vec2( textureSize( u_FontAtlases[ int( o_AtlasIndex ) ], 0 ) );

    vec2 screenTexSize = vec2( 1.0 ) / fwidth( o_InputData.TexCoord );
    return max( 0.5 * dot( unitRange, screenTexSize ), 1.0 );
}

void main() 
{
	vec4 bgColor = vec4( o_InputData.Color.rgb, 0.0 );
	vec4 fgColor = o_InputData.Color;

	vec3 msd = texture( u_FontAtlases[ int( o_AtlasIndex ) ], o_InputData.TexCoord ).rgb;
	float sd = Median( msd.r, msd.g, msd.b );
	float screenPxDistance = ScreenPxRange() * ( sd - 0.5 );
	float alpha = clamp( screenPxDistance + 0.5, 0.0, 1.0 );

	/*
	debugPrintfEXT( "sd: %f", sd );
	debugPrintfEXT( "screenPxDistance: %f", screenPxDistance );
	debugPrintfEXT( "alpha: %f", alpha );
	*/

	FinalColor = mix( bgColor, fgColor, alpha );
}
