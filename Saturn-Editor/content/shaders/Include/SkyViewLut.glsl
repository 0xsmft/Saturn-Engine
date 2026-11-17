//
// Sky-view LUT parameterisation
//

#type header
#pragma once

#ifndef SKY_PI
#define SKY_PI 3.14159265359
#endif

// Nearest positive hit of a ray with a sphere at the origin, -1 on a miss.
float SkyRaySphereNearest( vec3 ro, vec3 rd, float radius )
{
	float b = dot( ro, rd );
	float c = dot( ro, ro ) - radius * radius;
	float h = b * b - c;

	if( h < 0.0 )
		return -1.0;

	h = sqrt( h );
	float t0 = -b - h;
	float t1 = -b + h;

	if( t0 >= 0.0 ) return t0;
	if( t1 >= 0.0 ) return t1;

	return -1.0;
}

vec2 SkyViewLutParamsToUv( bool intersectGround, float viewZenithCosAngle, float lightViewCosAngle, float viewHeight, float bottomRadius )
{
	float vHorizon = sqrt( max( viewHeight * viewHeight - bottomRadius * bottomRadius, 0.0 ) );
	float cosBeta = vHorizon / viewHeight;
	float beta = acos( clamp( cosBeta, -1.0, 1.0 ) );
	float zenithHorizonAngle = SKY_PI - beta;

	vec2 uv;

	if( !intersectGround )
	{
		float coord = acos( clamp( viewZenithCosAngle, -1.0, 1.0 ) ) / zenithHorizonAngle;
		coord = 1.0 - coord;
		coord = sqrt( max( coord, 0.0 ) );
		coord = 1.0 - coord;
		uv.y = coord * 0.5;
	}
	else
	{
		float coord = ( acos( clamp( viewZenithCosAngle, -1.0, 1.0 ) ) - zenithHorizonAngle ) / max( beta, 1e-5 );
		coord = sqrt( max( coord, 0.0 ) );
		uv.y = coord * 0.5 + 0.5;
	}

	{
		float coord = -lightViewCosAngle * 0.5 + 0.5;
		coord = sqrt( max( coord, 0.0 ) );
		uv.x = coord;
	}

	return clamp( uv, vec2( 0.0 ), vec2( 1.0 ) );
}

void UvToSkyViewLutParams( vec2 uv, float viewHeight, float bottomRadius, out float viewZenithCosAngle, out float lightViewCosAngle )
{
	float vHorizon = sqrt( max( viewHeight * viewHeight - bottomRadius * bottomRadius, 0.0 ) );
	float cosBeta = vHorizon / viewHeight;
	float beta = acos( clamp( cosBeta, -1.0, 1.0 ) );
	float zenithHorizonAngle = SKY_PI - beta;

	if( uv.y < 0.5 )
	{
		float coord = 2.0 * uv.y;
		coord = 1.0 - coord;
		coord *= coord;
		coord = 1.0 - coord;
		viewZenithCosAngle = cos( zenithHorizonAngle * coord );
	}
	else
	{
		float coord = uv.y * 2.0 - 1.0;
		coord *= coord;
		viewZenithCosAngle = cos( zenithHorizonAngle + beta * coord );
	}

	float coord = uv.x;
	coord *= coord;
	lightViewCosAngle = -( coord * 2.0 - 1.0 );
}

// Image2D samplers are NEAREST, filter by hand.
vec3 SkySampleBilinear( sampler2D tex, vec2 uv )
{
	ivec2 size = textureSize( tex, 0 );
	vec2 p = uv * vec2( size ) - 0.5;
	ivec2 i = ivec2( floor( p ) );
	vec2 f = p - vec2( i );
	ivec2 maxP = size - 1;

	vec3 a = texelFetch( tex, clamp( i,                ivec2( 0 ), maxP ), 0 ).rgb;
	vec3 b = texelFetch( tex, clamp( i + ivec2( 1, 0 ), ivec2( 0 ), maxP ), 0 ).rgb;
	vec3 c = texelFetch( tex, clamp( i + ivec2( 0, 1 ), ivec2( 0 ), maxP ), 0 ).rgb;
	vec3 d = texelFetch( tex, clamp( i + ivec2( 1, 1 ), ivec2( 0 ), maxP ), 0 ).rgb;

	return mix( mix( a, b, f.x ), mix( c, d, f.x ), f.y );
}

// Sky luminance for an illuminance of 1 coming from sunDir, seen from cameraPosKm along dir.
vec3 SampleSkyViewLut( sampler2D lut, vec3 cameraPosKm, vec3 dir, vec3 sunDir, float bottomRadius )
{
	float viewHeight = max( length( cameraPosKm ), bottomRadius + 0.001 );
	vec3 up = cameraPosKm / max( length( cameraPosKm ), 1e-5 );

	float viewZenithCosAngle = dot( dir, up );

	// Angle between the view and the sun, measured around the up axis.
	vec3 side = cross( up, dir );
	float sideLen = length( side );
	float lightViewCosAngle = 1.0;

	if( sideLen > 1e-5 )
	{
		side /= sideLen;
		vec3 forward = normalize( cross( side, up ) );
		vec2 lightOnPlane = vec2( dot( sunDir, forward ), dot( sunDir, side ) );
		float len = length( lightOnPlane );
		lightViewCosAngle = len > 1e-5 ? lightOnPlane.x / len : 1.0;
	}

	bool intersectGround = SkyRaySphereNearest( up * viewHeight, dir, bottomRadius ) >= 0.0;

	vec2 uv = SkyViewLutParamsToUv( intersectGround, viewZenithCosAngle, lightViewCosAngle, viewHeight, bottomRadius );
	return SkySampleBilinear( lut, uv );
}

// Average sky luminance over the upper hemisphere (sun illuminance of 1), from the zenith
// and a ring of directions 30 degrees above the horizon. A Lambertian surface under a sky
// of uniform luminance L reflects albedo * L, so this is also the sky light on the ground.
vec3 SkyHemisphereAverage( sampler2D lut, vec3 cameraPosKm, vec3 sunDir, float bottomRadius )
{
	vec3 up = normalize( cameraPosKm );
	vec3 tangent = normalize( abs( up.y ) < 0.99 ? cross( up, vec3( 0.0, 1.0, 0.0 ) ) : cross( up, vec3( 1.0, 0.0, 0.0 ) ) );
	vec3 bitangent = cross( up, tangent );

	vec3 sum = SampleSkyViewLut( lut, cameraPosKm, up, sunDir, bottomRadius );

	const float kElevation = 0.5236; // 30 degrees
	for( int i = 0; i < 8; ++i )
	{
		float a = float( i ) * ( 2.0 * SKY_PI / 8.0 );
		vec3 dir = normalize( up * sin( kElevation ) + ( tangent * cos( a ) + bitangent * sin( a ) ) * cos( kElevation ) );
		sum += SampleSkyViewLut( lut, cameraPosKm, dir, sunDir, bottomRadius );
	}

	return sum / 9.0;
}
