//
// Physically based sky (Hillaire 2020), drawn in place of the skybox cubemap.
// Looks up the sky-view LUT for the view direction and adds the sun disk.
//

#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 vs_Ndc;

void main()
{
	vs_Ndc = a_Position.xy;
	gl_Position = vec4( a_Position.xy, 1.0, 1.0 );
}

#type fragment
#version 450

#include <SkyViewLut.glsl>

layout(location = 0) out vec4 FinalColor;
// The geo pass has two extra outputs that we don't write to.
layout(location = 1) out vec4 UnusedA;
layout(location = 2) out vec4 UnusedB;

layout(location = 0) in vec2 vs_Ndc;

layout(binding = 1) uniform sampler2D u_SkyViewLut;
layout(binding = 2) uniform sampler2D u_TransmittanceLut;

layout(binding = 3) uniform SkyData
{
	mat4 InverseViewProj;
	vec4 CameraPosKm;       // Planet centred
	vec4 SunDirection;      // Towards the sun
	vec4 SkyLight;          // rgb = sun illuminance used for the sky
	vec4 SunDisk;           // x = cos( angular radius ), y = disk intensity
	vec4 Radii;             // x = bottom radius, y = top radius (km)
	vec4 GroundAlbedo;
} u_Sky;

// Approximate extinction of the air near sea level (Rayleigh + Mie, 1/km), used to fade
// the ground into the haze with distance.
const vec3 kGroundLevelExtinction = vec3( 0.0102, 0.0180, 0.0375 );

// Must match LutTransmittanceParamsToUv in AtmosphereCommon.glsl.
vec2 TransmittanceUv( float viewHeight, float viewZenithCosAngle )
{
	float bottom = u_Sky.Radii.x;
	float top = u_Sky.Radii.y;

	float H = sqrt( max( 0.0, top * top - bottom * bottom ) );
	float rho = sqrt( max( 0.0, viewHeight * viewHeight - bottom * bottom ) );

	float discriminant = viewHeight * viewHeight * ( viewZenithCosAngle * viewZenithCosAngle - 1.0 ) + top * top;
	float d = max( 0.0, ( -viewHeight * viewZenithCosAngle + sqrt( max( discriminant, 0.0 ) ) ) );

	float dMin = top - viewHeight;
	float dMax = rho + H;

	return vec2( ( d - dMin ) / ( dMax - dMin ), rho / H );
}

void main()
{
	vec4 nearClip = u_Sky.InverseViewProj * vec4( vs_Ndc, 0.0, 1.0 );
	vec4 farClip  = u_Sky.InverseViewProj * vec4( vs_Ndc, 1.0, 1.0 );
	vec3 dir = normalize( farClip.xyz / farClip.w - nearClip.xyz / nearClip.w );

	vec3 sunDir = normalize( u_Sky.SunDirection.xyz );
	float bottomRadius = u_Sky.Radii.x;

	vec3 cameraPosKm = u_Sky.CameraPosKm.xyz;
	float viewHeight = max( length( cameraPosKm ), bottomRadius + 0.01 );
	vec3 up = cameraPosKm / max( length( cameraPosKm ), 1e-5 );
	cameraPosKm = up * viewHeight;

	vec3 luminance = SampleSkyViewLut( u_SkyViewLut, cameraPosKm, dir, sunDir, bottomRadius ) * u_Sky.SkyLight.rgb;

	float tGround = SkyRaySphereNearest( cameraPosKm, dir, bottomRadius );

	if( tGround >= 0.0 )
	{
		vec3 skyLight = SkyHemisphereAverage( u_SkyViewLut, cameraPosKm, sunDir, bottomRadius ) * u_Sky.SkyLight.rgb;
		vec3 throughput = exp( -kGroundLevelExtinction * tGround );

		luminance += throughput * u_Sky.GroundAlbedo.rgb * skyLight;
	}

	float cosToSun = dot( dir, sunDir );
	if( cosToSun > u_Sky.SunDisk.x && tGround < 0.0 )
	{
		vec2 uv = TransmittanceUv( viewHeight, dot( sunDir, up ) );
		vec3 transmittance = SkySampleBilinear( u_TransmittanceLut, clamp( uv, vec2( 0.0 ), vec2( 1.0 ) ) );

		// Soft limb so the disk edge doesn't alias.
		float edge = smoothstep( u_Sky.SunDisk.x, mix( u_Sky.SunDisk.x, 1.0, 0.3 ), cosToSun );

		luminance += transmittance * u_Sky.SkyLight.rgb * u_Sky.SunDisk.y * edge;
	}

	FinalColor = vec4( luminance, 1.0 );
}
