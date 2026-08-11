// Scene Composite shader

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
	vec4 position = vec4( vs_Output.TexCoord * 2.0f + -1.0f, 0.0, 1.0 );
	
	vs_Output.Position = position.xyz;
	gl_Position = position;
}

#type fragment
#version 450

layout (binding = 0) uniform sampler2D u_GeometryPassTexture;
layout (binding = 1) uniform sampler2D u_BloomTexture;
layout (binding = 2) uniform sampler2D u_AOTexture;
layout (binding = 3) uniform sampler2D u_DepthTexture;

layout(location = 0) out vec4 FinalColor;

layout(location = 1) in VertexOutput 
{
	vec3 Position;
	vec2 TexCoord;
} vs_Input;

vec3 GammaCorrect( vec3 color, float gamma ) 
{
	return pow( color, vec3( 1.0f / gamma ) );
}

vec3 UpsampleTent9(sampler2D tex, float lod, vec2 uv, vec2 texelSize, float radius)
{
	vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;

	// Center
	vec3 result = textureLod(tex, uv, lod).rgb * 4.0f;

	result += textureLod(tex, uv - offset.xy, lod).rgb;
	result += textureLod(tex, uv - offset.wy, lod).rgb * 2.0;
	result += textureLod(tex, uv - offset.zy, lod).rgb;

	result += textureLod(tex, uv + offset.zw, lod).rgb * 2.0;
	result += textureLod(tex, uv + offset.xw, lod).rgb * 2.0;

	result += textureLod(tex, uv + offset.zy, lod).rgb;
	result += textureLod(tex, uv + offset.wy, lod).rgb * 2.0;
	result += textureLod(tex, uv + offset.xy, lod).rgb;

	return result * (1.0f / 16.0f);
}

vec3 Reinhard( vec3 x )
{
	return x / (1.0 + x);
}

float Luminance( vec3 colr )  
{
	return dot( colr, vec3(0.2126, 0.7152, 0.0722) );
}

vec3 WhiteLuminance( vec3 colr ) 
{
	const float pureWhite = 1.0;
	float l = Luminance( colr );
	float mappedLuminance = (l * (1.0 + l / (pureWhite * pureWhite))) / (1.0 + l);
	
	return mappedLuminance / l * colr;
}

vec3 ACES( vec3 colr ) 
{
	mat3 m1 = mat3( 0.59719, 0.07600, 0.02840, 0.35458, 0.90834, 0.13383, 0.04823, 0.01566, 0.83777 );
	mat3 m2 = mat3( 1.60475, -0.10208, -0.00327, -0.53108, 1.10813, -0.07276, -0.07367, -0.00605, 1.07602 );

	vec3 v = m1 * colr;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

layout(push_constant) uniform u_Params 
{
	uint Flags;
	uint ColourBlindMode;
} pc_Params;

const uint SceneCompositeFlag_GTAO = 0x1u;
const uint SceneCompositeFlag_NoBloom = 0x2u;

const uint ColourBlindMode_None = 0u;
const uint ColourBlindMode_Protanope = 1u;
const uint ColourBlindMode_Deuteranope = 2u;
const uint ColourBlindMode_Tritanope = 3u;

// https://gist.github.com/jcdickinson/580b7fb5cc145cee8740
vec3 ColourBlind( vec3 color ) 
{
	float L = 17.8824 * color.r +
			  43.5161 * color.g +
			  4.11935 * color.b;

	float M = 3.45565 * color.r +
			  27.1554 * color.g +
			  3.86714 * color.b;

	float S = 0.0299566 * color.r +
			  0.184309 * color.g +
			  1.46709 * color.b;

	float l;
	float m;
	float s;

	switch( pc_Params.ColourBlindMode )
	{
		case ColourBlindMode_None: 
			return color;

		case ColourBlindMode_Protanope:
		{
			l = 2.02344 * M - 2.52581 * S;
			m = M;
			s = S;
		} break;

		case ColourBlindMode_Deuteranope:
		{
			l = L;
			m = 0.494207 * L + 1.24827 * S;
			s = S;
		} break;

		case ColourBlindMode_Tritanope:
		{
			l = L;
			m = M;
			s = -0.395913 * L + 0.801109 * M;
		} break;
	}

	vec3 simulated;

	simulated.r =
		0.0809444479 * l -
		0.130504409  * m +
		0.116721066  * s;

	simulated.g =
		-0.0102485335 * l +
		0.0540193266  * m -
		0.113614708   * s;

	simulated.b =
		-0.000365296938 * l -
		0.00412161469  * m +
		0.693511405    * s;

	vec3 error = color.rgb - simulated;

	// Daltonization correction
	vec3 correction;

	correction.r = 0.0;

	correction.g =
		error.r * 0.7 +
		error.g * 1.0;

	correction.b =
		error.r * 0.7 +
		error.b * 1.0;

	// Apply correction
	vec3 result = color.rgb + correction;

	return vec3( clamp( result, 0.0, 1.0 ) );
}

void main()
{
	const float GAMMA = 2.2;

	vec3 GeometryPassColor = texture( u_GeometryPassTexture, vs_Input.TexCoord ).rgb;

	// Bloom Composite
	float sampleScale = 0.5;
	ivec2 texSize = textureSize(u_BloomTexture, 0);

	if( ( pc_Params.Flags & SceneCompositeFlag_NoBloom ) == 0 )
	{
		vec2 fTexSize = vec2( float( texSize.x ), float( texSize.y ) );
		vec3 bloom = UpsampleTent9( u_BloomTexture, 0, vs_Input.TexCoord, 1.0f / fTexSize, sampleScale );

		GeometryPassColor += bloom;
	}

	GeometryPassColor = ACES( GeometryPassColor );
	GeometryPassColor = GammaCorrect( GeometryPassColor, GAMMA );

	if( ( pc_Params.Flags & SceneCompositeFlag_GTAO ) != 0 )
	{
		float ao = clamp( texture( u_AOTexture, vs_Input.TexCoord ).r, 0.0, 1.0 );
		ao = min( ao * 1.5, 1.0 );
		GeometryPassColor *= ao;
	}
	else
	{
		float ao = texture( u_AOTexture, vs_Input.TexCoord ).r; 
		GeometryPassColor *= ao;
	}

	GeometryPassColor = ColourBlind( GeometryPassColor );

	FinalColor = vec4( GeometryPassColor, 1.0 );
}
