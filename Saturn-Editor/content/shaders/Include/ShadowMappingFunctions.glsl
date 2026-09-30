//
// Shadow mapping functions for composition shaders.
//

#type header
#pragma once

#include <SharedBuffers.glsl>

float GetShadowBias() 
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	float bias = max( MINIMUM_SHADOW_BIAS * ( 1.0 - dot( m_Params.Normal, u_Camera.DirectionalLight.Direction ) ), MINIMUM_SHADOW_BIAS );
	return bias;
}

float HardShadows( sampler2DArray ShadowMap, vec3 ShadowCoords, uint index ) 
{
	float bias = GetShadowBias();
	vec2 texelSize = 1.0 / textureSize( ShadowMap, 0 ).xy;
	vec2 invShadowMapSize = 1.0 / vec2( textureSize( ShadowMap, 0 ) );

	float map = texture( ShadowMap, vec3( ShadowCoords.xy * 0.5 + 0.5, index ) ).x;

	// TEMP: Soft Shadows?
	float shadow = 0.0;
	float filterSize = 4.0 / 2;

	for( float x = -filterSize; x <= filterSize; x++ )
    {
        for( float y = -filterSize; y <= filterSize; y++ )
        {
            vec2 offset = vec2( x, y ) * texelSize;
            float text = texture( ShadowMap, vec3( ShadowCoords.xy * 0.5 + 0.5 + offset, index ) ).x;
            shadow += step( ShadowCoords.z, text + bias );
        }
    }

    shadow /= ( ( 2.0 * filterSize + 1.0 ) * ( 2.0 * filterSize + 1.0 ) );
	return shadow;
}
