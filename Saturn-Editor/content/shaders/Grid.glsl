// Endless grid

#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(binding = 0) uniform GridData
{
	mat4 ViewProjection;
	mat4 InverseViewProjection;

	// xyz = camera position, w = distance where the grid has fully faded out.
	vec4 CameraPosition;

	// x = minor cell size, y = major cell size.
	vec4 CellSizes;
} u_Grid;

layout(location = 0) out VertexOutput
{
	vec3 NearPoint;
	vec3 FarPoint;
} vs_Output;

vec3 Unproject( vec2 ndc, float depth )
{
	vec4 point = u_Grid.InverseViewProjection * vec4( ndc, depth, 1.0 );
	return point.xyz / point.w;
}

void main()
{
	// The quad is already in NDC. 
	// Any two depths give us a ray through this pixel, 
	// so this does not depend on the depth convention.
	vs_Output.NearPoint = Unproject( a_Position.xy, 0.0 );
	vs_Output.FarPoint = Unproject( a_Position.xy, 1.0 );

	gl_Position = vec4( a_Position.xy, 0.0, 1.0 );
}

#type fragment
#version 450

layout(location = 0) out vec4 FinalColor;

// Unsued outputs from geo pass.
layout(location = 1) out vec4 UnusedA;
layout(location = 2) out vec4 UnusedB;

layout(binding = 0) uniform GridData
{
	mat4 ViewProjection;
	mat4 InverseViewProjection;
	vec4 CameraPosition;
	vec4 CellSizes;
} u_Grid;

layout(location = 0) in VertexOutput
{
	vec3 NearPoint;
	vec3 FarPoint;
} vs_Input;

const vec3 kMinorColour = vec3( 0.2 );
const vec3 kMajorColour = vec3( 0.3 );
const vec3 kXAxisColour = vec3( 0.8, 0.15, 0.15 );
const vec3 kZAxisColour = vec3( 0.15, 0.3, 0.85 );

const float kMinorAlpha = 0.35;
const float kMajorAlpha = 0.6;
const float kAxisAlpha = 0.9;

float GridCoverage( vec2 worldXZ, float cellSize )
{
	vec2 coord = worldXZ / cellSize;
	vec2 derivative = max( fwidth( coord ), vec2( 1e-6 ) );

	// Distance to the nearest line, in pixels.
	vec2 distanceToLine = abs( fract( coord - 0.5 ) - 0.5 ) / derivative;
	float coverage = 1.0 - min( min( distanceToLine.x, distanceToLine.y ), 1.0 );

	float cellSizeInPixels = 1.0 / max( derivative.x, derivative.y );

	return coverage * smoothstep( 2.0, 8.0, cellSizeInPixels );
}

// How much of this pixel is covered by the line where value == 0.
float AxisCoverage( float value )
{
	float derivative = max( fwidth( value ), 1e-6 );
	return 1.0 - clamp( abs( value ) / ( derivative * 1.5 ), 0.0, 1.0 );
}

void main()
{
	// Intersect the ray with the y = 0 plane.
	vec3 rayDirection = vs_Input.FarPoint - vs_Input.NearPoint;
	float rayY = abs( rayDirection.y ) < 1e-6 ? ( rayDirection.y < 0.0 ? -1e-6 : 1e-6 ) : rayDirection.y;

	float t = -vs_Input.NearPoint.y / rayY;
	vec3 worldPosition = vs_Input.NearPoint + t * rayDirection;

	vec4 clipPosition = u_Grid.ViewProjection * vec4( worldPosition, 1.0 );
	float depth = clipPosition.z / clipPosition.w;

	vec2 xz = worldPosition.xz;

	float minor = GridCoverage( xz, u_Grid.CellSizes.x ) * kMinorAlpha;
	float major = GridCoverage( xz, u_Grid.CellSizes.y ) * kMajorAlpha;

	vec3 colour = major >= minor ? kMajorColour : kMinorColour;
	float alpha = max( minor, major );

	float xAxis = AxisCoverage( xz.y );
	float zAxis = AxisCoverage( xz.x );

	colour = mix( colour, kXAxisColour, xAxis );
	alpha = max( alpha, xAxis * kAxisAlpha );

	colour = mix( colour, kZAxisColour, zAxis );
	alpha = max( alpha, zAxis * kAxisAlpha );

	// Fade out with distance from the camera.
	float distanceFromCamera = length( xz - u_Grid.CameraPosition.xz );
	float fadeDistance = u_Grid.CameraPosition.w;
	alpha *= 1.0 - smoothstep( fadeDistance * 0.25, fadeDistance, distanceFromCamera );

	// Looking away from the plane, beyond the far plane or too faint to see.
	if( t <= 0.0 || depth < 0.0 || depth > 1.0 || alpha <= 0.001 )
		discard;

	gl_FragDepth = depth;

	FinalColor = vec4( colour, alpha );
	UnusedA = vec4( 0.0 );
	UnusedB = vec4( 0.0 );
}
