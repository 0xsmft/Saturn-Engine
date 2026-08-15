// Alura 2D quad shader

#type vertex
#version 450

// Inputs
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec2 a_RectSize;
layout(location = 3) in vec4 a_Color;
layout(location = 4) in float a_TextureIndex;
layout(location = 5) in float a_Rounding;

layout(push_constant) uniform pc_ScalingAndTransfrom
{
	mat4 Projection;
} u_Transform; 

struct VertOut 
{
	vec2 TexCoord;
	vec2 RectSize;
	vec4 Color;
	float Rounding;
};

layout( location = 0 ) out VertOut o_OutputData;
layout( location = 5 ) out flat float o_TexIndex;

void main() 
{
	o_OutputData.Color = a_Color;
	o_OutputData.TexCoord = a_TexCoord;
	o_OutputData.RectSize = a_RectSize;
	o_OutputData.Rounding = a_Rounding;

	o_TexIndex = a_TextureIndex;

	gl_Position = u_Transform.Projection * vec4( a_Position, 0.0, 1.0 );
}

#type fragment
#version 450

struct VertOut 
{
	vec2 TexCoord;
	vec2 RectSize;
	vec4 Color;
	float Rounding;
};

// Inputs
layout( location = 0 ) in VertOut o_InputData;
layout( location = 5 ) in flat float o_TexIndex;

// Uniforms
layout( set = 0, binding = 1 ) uniform sampler2D u_InputTexture[16];

// Outputs
layout( location = 0 ) out vec4 FinalColor;

// Do I know what this does? No.
// But if you want to read up:
// Use - https://medium.com/@solidalloy/drawing-rounded-corners-and-borders-with-sdf-part-1-rounded-corners-8017bb6ce6f9
// and search for "Let's start with the canonical form of the rounded corner SDF function:"
float SDRoundBox( vec2 p, vec2 halfSize, float radius )
{
	vec2 q = abs( p ) - halfSize + radius;
	return length( max( q, 0.0 ) ) + min( max( q.x, q.y ), 0.0 ) - radius;
}

void main() 
{
	vec2 halfSize = o_InputData.RectSize * 0.5;
	vec2 p = ( o_InputData.TexCoord - 0.5 ) * o_InputData.RectSize;

	float d = SDRoundBox( p, halfSize, o_InputData.Rounding );

	float aa = max( fwidth( d ), 0.001 );
	float alpha = 1.0 - smoothstep( 0.0, aa, d );

	FinalColor = texture( u_InputTexture[ int( o_TexIndex ) ], o_InputData.TexCoord.st ) * o_InputData.Color;
	FinalColor.a *= alpha;
}
