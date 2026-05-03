#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 1) out VertexOutput 
{
	vec2 TexCoord;
	vec2 TexelSize;
	vec2 UV[9];
} vs_Output;

layout(push_constant) uniform RenderData 
{
	vec2 TexelSize;
	int StepSize;
} u_Data;

void main() 
{
	vs_Output.TexCoord = a_TexCoord;
	vs_Output.TexelSize = u_Data.TexelSize;

	vec2 dx = vec2( u_Data.TexelSize.x, 0.0f ) * u_Data.StepSize;
	vec2 dy = vec2( 0.0f, u_Data.TexelSize.y ) * u_Data.StepSize;

	vs_Output.UV[0] = a_TexCoord;

	vs_Output.UV[1] = a_TexCoord + dx;
	vs_Output.UV[2] = a_TexCoord - dx;

	vs_Output.UV[3] = a_TexCoord + dy;
	vs_Output.UV[4] = a_TexCoord - dy;

	vs_Output.UV[5] = a_TexCoord + dx + dy;
	vs_Output.UV[6] = a_TexCoord + dx - dy;

	vs_Output.UV[7] = a_TexCoord - dx + dy;
	vs_Output.UV[8] = a_TexCoord - dx - dy;

	gl_Position = vec4( a_Position, 1.0 );	
}

#type fragment
#version 450

layout(location = 0) out vec4 FinalColor;

layout(location = 1) in VertexOutput 
{
	vec2 TexCoord;
	vec2 TexelSize;
	vec2 UV[9];
} vs_Input;

layout (set = 0, binding = 2) uniform sampler2D u_InputTexture;

float ScreenDistance( vec2 v ) 
{
	float ratio = vs_Input.TexelSize.x / vs_Input.TexelSize.y;

	v.x /= ratio;

	return dot( v, v );
}

void BoundsCheck( inout vec2 xy, vec2 uv ) 
{
	if( uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f )
		xy = vec2( 1000.0f );
}

void main()
{
	vec4 color = texture( u_InputTexture, vs_Input.UV[ 0 ] );

	for( int i = 1; i <= 8; ++i ) 
	{
		vec4 n = texture( u_InputTexture, vs_Input.UV[ i ] );
		if( n.w != color.w )
			n.xyz = vec3( 0.0f );

		n.xy += vs_Input.UV[ i ] - vs_Input.UV[ 0 ];

		BoundsCheck( n.xy, vs_Input.UV[ i ] );

		float dist = ScreenDistance( n.xy );
		if( dist < color.z )
			color.xyz = vec3( n.xy, dist );
	}

	FinalColor = color;
}
