#type vertex
#version 450

// Inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TextureIndex;

layout(set = 0, binding = 0) uniform Matrices
{
	mat4 ViewProjection;
} u_Matrices;

layout(push_constant) uniform pc_Transform
{
	mat4 Transform;
} u_Transform; 

struct VertOut 
{
	vec2 TexCoord;
	vec4 Color;
};

layout( location = 0 ) out VertOut o_OutputData;
layout( location = 2 ) out flat float o_TexIndex;

void main() 
{
	o_OutputData.Color = a_Color;
	o_OutputData.TexCoord = a_TexCoord;
	o_TexIndex = a_TextureIndex;

	gl_Position = u_Matrices.ViewProjection * u_Transform.Transform * vec4( a_Position, 1.0 );
}

#type fragment
#version 450

struct VertOut 
{
	vec2 TexCoord;
	vec4 Color;
};

layout( location = 0 ) in VertOut o_InputData;
layout( location = 2 ) in flat float o_TexIndex;

layout( location = 0 ) out vec4 FinalColor;

layout(set = 0, binding = 0) uniform Matrices
{
	mat4 ViewProjection;
} u_Matrices;

layout( set = 0, binding = 1 ) uniform sampler2D u_InputTexture[16];

void main() 
{
	FinalColor = texture( u_InputTexture[ int(o_TexIndex) ], o_InputData.TexCoord ) * o_InputData.Color;

	if( FinalColor.a == 0.0 )
		discard;
}