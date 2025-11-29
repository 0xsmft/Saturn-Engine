#type vertex
#version 450

// Inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Bitangent;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 5) in vec4 a_TransformBufferR1;
layout(location = 6) in vec4 a_TransformBufferR2;
layout(location = 7) in vec4 a_TransformBufferR3;
layout(location = 8) in vec4 a_TransformBufferR4;

layout(location = 9) in ivec4 a_BoneIndices;
layout(location = 10) in vec4 a_BoneWeights;

layout(set = 0, binding = 0) uniform Matrices
{
	mat4 ViewProjection;
} u_Matrices;

// Set 2, owned by renderer, bone data.
layout(std430, set = 1, binding = 15) readonly buffer AnimationBoneData
{
	// 100 MAX * 1024
	mat4 Transforms[102400];
} s_AnimationBoneData;

layout(push_constant) uniform pc_MeshIndex
{
    uint Index;
} u_MeshIndex;

void main() 
{
	mat4 skinMatrix = s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.x ] * a_BoneWeights.x;
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.y ] * a_BoneWeights.y; 
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.z ] * a_BoneWeights.z;
	skinMatrix += s_AnimationBoneData.Transforms[ ( u_MeshIndex.Index + gl_InstanceIndex ) * 100 + a_BoneIndices.w ] * a_BoneWeights.w;

	mat4 transform = mat4( 
		a_TransformBufferR1.x, a_TransformBufferR2.x, a_TransformBufferR3.x, 0.0, 
		a_TransformBufferR1.y, a_TransformBufferR2.y, a_TransformBufferR3.y, 0.0, 
		a_TransformBufferR1.z, a_TransformBufferR2.z, a_TransformBufferR3.z, 0.0, 
		a_TransformBufferR1.w, a_TransformBufferR2.w, a_TransformBufferR3.w, 1.0);

	vec4 worldPos = transform * skinMatrix * vec4( a_Position, 1.0 );

	gl_Position = u_Matrices.ViewProjection * worldPos;
}
