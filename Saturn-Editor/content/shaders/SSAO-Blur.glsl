#type vertex
#version 450

// Inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// Outputs
layout(location = 0) out vec2 o_TexCoord;

void main() 
{	
	vec4 position = vec4( a_Position.xy, 0.0, 1.0 );
	gl_Position = position;

	o_TexCoord = a_TexCoord;
}

#type fragment
#version 450

layout( location = 0 ) in vec2 vs_UV;

layout( set = 0, binding = 1 ) uniform sampler2D u_AOTexture;

layout( location = 0 ) out float FinalColorR;

void main() 
{
	const int blurRange = 2;

	int n = 0;

	vec2 texelSize = 1.0 / vec2( textureSize( u_AOTexture, 0 ) );

	float result = 0.0f;
	for( int x = -blurRange; x <= blurRange; ++x ) 
	{
		for( int y = -blurRange; y <= blurRange; ++y ) 
		{
			vec2 offset = vec2( float( x ), float( y ) ) * texelSize;
			result += texture( u_AOTexture, vs_UV + offset ).r;

			++n;
		}
	}

	FinalColorR = result / (float(n));
}
