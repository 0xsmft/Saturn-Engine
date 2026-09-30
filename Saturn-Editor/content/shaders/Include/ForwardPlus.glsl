//
// Forward plus lighting functions.
//

#type header
#pragma once

#include <SharedBuffers.glsl>
#include <Lighting.glsl>

//////////////////////////////////////////////////////////////////////////
// Forward+
int GetPointLightBufferIndex(int i)
{
	ivec2 tileID = ivec2(gl_FragCoord) / ivec2(16, 16);
	uint index = tileID.y * u_DebugData.TilesCountX + tileID.x;

	uint offset = index * 1024;
	return s_VisiblePointLightIndicesBuffer.Indices[offset + i];
}

int GetPointLightCount()
{
	int result = 0;
	for (int i = 0; i < u_Lights.nbLights; i++)
	{
		uint lightIndex = GetPointLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		result++;
	}

	return result;
}


//////////////////////////////////////////////////////////////////////////
// Forward+, Point Lights

vec3 CalculatePointLights(in vec3 F0, vec3 worldPos)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_Lights.nbLights; i++)
	{
		uint lightIndex = GetPointLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		PointLight light = u_Lights.Lights[lightIndex];
		vec3 Li = normalize(light.Position - worldPos);
		float lightDistance = length(light.Position - worldPos);
		vec3 Lh = normalize(Li + m_Params.View);

		float attenuation = clamp(1.0 - lightDistance * lightDistance / (light.Radius * light.Radius * 10), 0.0, 1.0);
		attenuation *= mix(attenuation, 1.0, light.Falloff);

		vec3 Lradiance = light.Radiance * light.Multiplier * attenuation;

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
		float D = NDFGGX(cosLh, m_Params.Roughness);
		float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	
	return result;
}
