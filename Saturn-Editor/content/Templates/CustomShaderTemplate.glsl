#type vertex
#version 450 core

#include <GameShaders.glsl>
#include <ShadowMappingFunctions.glsl>
#include <Lighting.glsl>
#include <ImageBasedLighting.glsl>
#include <ForwardPlus.glsl>

struct VertexOutput 
{
	vec3 Normal;
	vec3 Bionormal;
	vec3 Position;
	vec2 TexCoord;
	mat3 WorldNormals;
	
	mat3 CameraView;

	vec4 ShadowMapCoords[ 4 ];
	vec3 ViewPosition;
};

layout( location = 1 ) in VertexOutput vs_Input;

layout( location = 0 ) out vec4 FinalColor;
layout( location = 1 ) out vec4 OutViewNormals;
layout( location = 2 ) out vec4 OutAlbedo;

void main() 
{
    FinalColor = vec4( 1.0 );
    OutViewNormals = vec4( 1.0 );
    OutAlbedo = vec4( 1.0 );
}
