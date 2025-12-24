// Alura 2D quad shader

#type vertex
#version 450

// Inputs
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;

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

void main() 
{
	o_OutputData.Color = a_Color;
	o_OutputData.TexCoord = a_TexCoord;

	gl_Position = u_Transform.Projection * vec4( a_Position /* u_Transform.Scale + u_Transform.Translate*/, 0.0, 1.0 );
}

#type fragment
#version 450

struct VertOut 
{
	vec2 TexCoord;
	vec4 Color;
};

layout( location = 0 ) in VertOut o_InputData;

layout( location = 0 ) out vec4 FinalColor;

layout( set = 0, binding = 1 ) uniform sampler2D u_InputTexture;

void main() 
{
	FinalColor = texture( u_InputTexture, o_InputData.TexCoord.st ) * o_InputData.Color;
}
