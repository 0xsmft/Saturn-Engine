//
// IBL functions
//

#type header
#pragma once

#include <SharedBuffers.glsl>

vec3 RotateVectorAboutY( float angle, vec3 vec )
{
	angle = radians( angle );

	mat3x3 rotationMatrix ={ vec3( cos( angle ), 0.0, sin( angle ) ),
							vec3( 0.0, 1.0, 0.0 ),
							vec3( -sin( angle ), 0.0, cos( angle ) ) };

	return rotationMatrix * vec;
}

vec3 IBL( vec3 F0, vec3 Lr )
{
	vec3 irradiance = texture( u_EnvIrradianceTex, m_Params.Normal ).rgb;

	vec3 F = FresnelSchlickRoughness( F0, m_Params.NdotV, m_Params.Roughness );
	vec3 kd = ( 1.0 - F ) * ( 1.0 - m_Params.Metalness );
	vec3 diffuseIBL = m_Params.Albedo * irradiance;
	
	int envRadianceTexLevels = textureQueryLevels( u_EnvRadianceTex );
	float NoV = clamp( m_Params.NdotV, 0.0, 1.0 );
	vec3 R = 2.0 * dot( m_Params.View, m_Params.Normal ) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = textureLod( u_EnvRadianceTex, RotateVectorAboutY( 0.0, Lr ), ( m_Params.Roughness ) * envRadianceTexLevels ).rgb;
	
	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture( u_BRDFLUTTexture, vec2( m_Params.NdotV, 1.0 - m_Params.Roughness ) ).rg;
	vec3 specularIBL = specularIrradiance * ( F0 * specularBRDF.x + specularBRDF.y );
	
	return kd * diffuseIBL + specularIBL;
}
