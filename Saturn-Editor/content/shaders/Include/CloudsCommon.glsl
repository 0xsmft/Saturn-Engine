//
// Shared functions for volumetric clouds.
//

#type header
#pragma once

// Based from:
//	piyushslayer (Shadertoy): https://www.shadertoy.com/view/3dVXDc

// Hash by David_Hoskins
#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))

vec3 Hash33( vec3 p )
{
	uvec3 q = uvec3( ivec3( p ) ) * UI3;

	q = ( q.x ^ q.y ^ q.z ) * UI3;
	return -1. + 2. * vec3( q ) * UIF;
}

float Remap(float x, float a, float b, float c, float d)
{
	return ( ( ( x - a ) / ( b - a ) ) * ( d - c ) ) + c;
}

// Gradient noise by iq (modified to be tileable).
float GradientNoise( vec3 x, float freq )
{
	// grid
	vec3 p = floor( x );
	vec3 w = fract( x );
	
	// quintic interpolant
	vec3 u = w * w * w * ( w * ( w * 6. - 15. ) + 10. );

	// gradients
	vec3 ga = Hash33( mod( p + vec3( 0., 0., 0. ), freq ) );
	vec3 gb = Hash33( mod( p + vec3( 1., 0., 0. ), freq ) );
	vec3 gc = Hash33( mod( p + vec3( 0., 1., 0. ), freq ) );
	vec3 gd = Hash33( mod( p + vec3( 1., 1., 0. ), freq ) );
	vec3 ge = Hash33( mod( p + vec3( 0., 0., 1. ), freq ) );
	vec3 gf = Hash33( mod( p + vec3( 1., 0., 1. ), freq ) );
	vec3 gg = Hash33( mod( p + vec3( 0., 1., 1. ), freq ) );
	vec3 gh = Hash33( mod( p + vec3( 1., 1., 1. ), freq ) );
	
	// projections
	float va = dot( ga, w - vec3( 0., 0., 0. ) );
	float vb = dot( gb, w - vec3( 1., 0., 0. ) );
	float vc = dot( gc, w - vec3( 0., 1., 0. ) );
	float vd = dot( gd, w - vec3( 1., 1., 0. ) );
	float ve = dot( ge, w - vec3( 0., 0., 1. ) );
	float vf = dot( gf, w - vec3( 1., 0., 1. ) );
	float vg = dot( gg, w - vec3( 0., 1., 1. ) );
	float vh = dot( gh, w - vec3( 1., 1., 1. ) );
	
	// interpolation
	return va + 
		   u.x * ( vb - va ) + 
		   u.y * ( vc - va ) + 
		   u.z * ( ve - va ) + 
		   u.x * u.y * ( va - vb - vc + vd ) + 
		   u.y * u.z * ( va - vc - ve + vg ) + 
		   u.z * u.x * ( va - vb - ve + vf ) + 
		   u.x * u.y * u.z * ( -va + vb + vc - vd + ve - vf - vg + vh );
}

// Tileable 3D worley noise
float WorleyNoise( vec3 uv, float freq )
{    
	vec3 id = floor(uv );
	vec3 p = fract( uv );
	
	float minDist = 10000.;
	for( float x = -1.; x <= 1.; ++x )
	{
		for( float y = -1.; y <= 1.; ++y )
		{
			for( float z = -1.; z <= 1.; ++z )
			{
				vec3 offset = vec3( x, y, z );
				vec3 h = Hash33( mod( id + offset, vec3( freq ) ) ) * .5 + .5;
				h += offset;
				
				vec3 d = p - h;
				minDist = min( minDist, dot( d, d ) );
			}
		}
	}
	
	// inverted worley noise
	return 1. - minDist;
}

// Fbm for Perlin noise based on iq's blog
float PerlinFBM( vec3 p, float freq, int octaves )
{
	float G = exp2( -.85 );
	float amp = 1.;
	float noise = 0.;

	for( int i = 0; i < octaves; ++i )
	{
		noise += amp * GradientNoise( p * freq, freq );
		freq *= 2.;
		amp *= G;
	}
	
	return noise;
}

// Tileable Worley fbm inspired by Andrew Schneider's Real-Time Volumetric Cloudscapes
// chapter in GPU Pro 7.
float WorleyFBM( vec3 p, float freq ) 
{
	return WorleyNoise( p * freq, freq ) * .625 +
       	 WorleyNoise( p * freq * 2., freq * 2.) * .25 +
       	 WorleyNoise( p * freq * 4., freq * 4.) * .125;
}
