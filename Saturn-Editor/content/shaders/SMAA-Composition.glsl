// SMAA Composition

#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 1) out vec2 o_TexCoord;

void main()
{
	o_TexCoord = a_TexCoord;
	
	gl_Position = vec4( a_Position, 1.0 );
}

#type fragment
#version 450

layout(location = 0) out vec4 FinalColor;
layout(location = 1) in vec2 vs_TexCoord;

layout(set = 0, binding = 0) uniform sampler2D u_SMAAOutTexture;

void main()
{
	FinalColor = texture( u_SMAAOutTexture, vs_TexCoord );
}
