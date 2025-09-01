// Shadow Map shader

#type vertex
#version 430

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Bitangent;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 5) in vec4 a_TransformBufferR1;
layout(location = 6) in vec4 a_TransformBufferR2;
layout(location = 7) in vec4 a_TransformBufferR3;
layout(location = 8) in vec4 a_TransformBufferR4;

// Set and Binding must match with StaticMesh shader for our UniformBufferSet.
layout(set = 0, binding = 1) uniform LightData
{
	mat4 ViewProjection[4];
} u_LightData;

layout(push_constant) uniform u_Transform
{
	uint CascadeIndex;
};

void main()
{
	mat4 transform = mat4( 
		a_TransformBufferR1.x, a_TransformBufferR2.x, a_TransformBufferR3.x, a_TransformBufferR4.x, 
		a_TransformBufferR1.y, a_TransformBufferR2.y, a_TransformBufferR3.y, a_TransformBufferR4.y, 
		a_TransformBufferR1.z, a_TransformBufferR2.z, a_TransformBufferR3.z, a_TransformBufferR4.z, 
		a_TransformBufferR1.w, a_TransformBufferR2.w, a_TransformBufferR3.w, a_TransformBufferR4.w  );

	gl_Position = u_LightData.ViewProjection[ CascadeIndex ] * transform * vec4( a_Position, 1.0 );
}
