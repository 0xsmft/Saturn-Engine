// Texture Pass

#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 1) out VertexOutput 
{
	vec3 Position;
	vec2 TexCoord;
} vs_Output;

void main()
{
	vs_Output.Position = a_Position;
	vs_Output.TexCoord = a_TexCoord;

	// Flip textures.
	vs_Output.TexCoord = vec2( a_TexCoord.s, 1.0 - a_TexCoord.t );

	vec4 position = vec4( a_Position.xy, 0.0, 1.0 );
	
	gl_Position = position;
	gl_Position.y *= -1.0;
	gl_Position.z = ( gl_Position.z + gl_Position.w ) / 2.0;
}

#type fragment
#version 450

layout (binding = 0) uniform sampler2D u_InputTexture;

layout(location = 0) out vec4 FinalColor;

layout(location = 1) in VertexOutput 
{
	vec3 Position;
	vec2 TexCoord;
} vs_Input;

void main()
{
	FinalColor = texture( u_InputTexture, vs_Input.TexCoord );
}
