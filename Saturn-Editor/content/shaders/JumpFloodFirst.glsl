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

float ScreenDistance( vec2 v, vec2 texelSize ) 
{
	float ratio = texelSize.x / texelSize.y;

	v.x /= ratio;

	return dot( v, v );
}

void main()
{
	vec4 color = texture( u_InputTexture, vs_Input.TexCoord );

	ivec2 textureSize = textureSize( u_InputTexture, 0 );
	vec2 texelSize = vec2( 1.0f / float( textureSize.x ), 1.0f / float( textureSize.y ) );

	vec4 resultCol;
	resultCol.xy = vec2(100, 100);
	resultCol.z = ScreenDistance( resultCol.xy, texelSize );

	resultCol.w = color.a > 0.5f ? 1.0f : 0.0f;

	FinalColor = resultCol;
}
