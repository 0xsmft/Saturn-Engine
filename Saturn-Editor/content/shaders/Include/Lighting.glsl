//
// Common PBR lighting functions
//
// Schlick, Smith etc
//

#type header
#pragma once

#include <ConstantDefines.glsl>
#include <SharedBuffers.glsl>

float NDFGGX( float cosLh, float r )
{
	float a = r * r;
	float alphaSq = a * a;

	float denom = ( cosLh * cosLh ) * ( alphaSq - 1.0 ) + 1.0;
	return alphaSq / ( PI * denom * denom );
}

float GaSchlickG1( float cosTheta, float k )
{
	return cosTheta / ( cosTheta * ( 1.0 - k ) + k );
}

float GaSchlickGGX( float cosLi, float NdotV, float roughness )
{
	float r = roughness + 1.0;
	
	// Epic suggests using this roughness remapping for analytic lights.
	float k = ( r * r ) / 8.0; 
	return GaSchlickG1( cosLi, k ) * GaSchlickG1( NdotV, k );
}

float GeometrySchlickGGX( float NdotV, float R )
{
	float r = ( R + 1.0 );
	float k = ( r * r ) / 8.0;

	float nom = NdotV;
	float denom = NdotV * ( 1.0 - k ) + k ;

	return nom / denom;
}

float GeometrySmith( vec3 N, vec3 V, vec3 L, float roughness )
{
	float NdotV = max( dot( N, V ), 0.0 );
	float NdotL = max( dot( N, L ), 0.0 );
	float ggx2 = GeometrySchlickGGX( NdotV, roughness );
	float ggx1 = GeometrySchlickGGX( NdotL, roughness );

	return ggx1 * ggx2;
}

vec3 FresnelSchlick( vec3 F0, float cosTheta )
{
	return F0 + ( 1.0 - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}

vec3 FresnelSchlickRoughness( vec3 F0, float cosTheta, float roughness )
{
	return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}

vec3 Lighting( vec3 F0 ) 
{
	vec3 result = vec3( 0.0 );
	
	for( int i = 0; i < LightCount; i++ )
	{
		vec3 Li = u_Camera.DirectionalLight.Direction;
		vec3 Lradiance = u_Camera.DirectionalLight.Radiance * u_Camera.DirectionalLight.Multiplier;
		vec3 Lh = normalize( Li + m_Params.View );

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max( 0.0, dot( m_Params.Normal, Li ) );
		float cosLh = max( 0.0, dot( m_Params.Normal, Lh ) );

		vec3 F = FresnelSchlick( F0, max( 0.0, dot( Lh, m_Params.View ) ) );
		float D = NDFGGX( cosLh, m_Params.Roughness );
		float G = GaSchlickGGX( cosLi, m_Params.NdotV, m_Params.Roughness );

		vec3 kd = ( 1.0 - F ) * ( 1.0 - m_Params.Metalness );
		vec3 DiffuseBRDF = kd * m_Params.Albedo;

		vec3 SpecularBRDF = ( F * D * G ) / max( Epsilon, 4.0 * cosLi * m_Params.NdotV );
		result += ( DiffuseBRDF + SpecularBRDF ) * Lradiance * cosLi;
	}

	return result;
}
