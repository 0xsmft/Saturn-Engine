// Texture Pass

#type vertex
#version 450

layout(location = 1) out VertexOutput 
{
	vec3 Position;
	vec2 TexCoord;
} vs_Output;

void main()
{
	vs_Output.TexCoord = vec2( ( gl_VertexIndex << 1 ) & 2, gl_VertexIndex & 2 );

	// Flip textures.
	vs_Output.TexCoord = vec2( vs_Output.TexCoord.s, 1.0 - vs_Output.TexCoord.t );

	vec4 position = vec4( vs_Output.TexCoord * 2.0 - 1.0, 0.0, 1.0 );
	
	vs_Output.Position = position;
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
