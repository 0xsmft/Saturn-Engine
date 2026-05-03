#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 1) out VertexOutput 
{
	vec2 TexCoord;
} vs_Output;

void main() 
{
	vs_Output.TexCoord = a_TexCoord;
	gl_Position = vec4( a_Position, 1.0 );	
}

#type fragment
#version 450

layout(location = 0) out vec4 FinalColor;

layout(location = 1) in VertexOutput 
{
	vec2 TexCoord;
} vs_Input;

layout (set = 0, binding = 2) uniform sampler2D u_InputTexture;

void main()
{
	vec4 color = texture( u_InputTexture, vs_Input.TexCoord );

	float dist = sqrt(color.z);
	float a = smoothstep(0.004f, 0.002f, dist);
	if( a == 0.0 )
		discard;

	vec3 outlineColor = vec3( 0.0f, 0.0f, 1.0f );
	FinalColor = vec4( outlineColor, a );
}
