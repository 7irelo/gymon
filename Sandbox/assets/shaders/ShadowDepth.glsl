// Depth-only pass for directional shadow mapping.
//
// Renders geometry from the light's point of view. There is no fragment
// output: the depth buffer is the entire product, and an empty main() lets
// the driver write gl_FragDepth automatically.

#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Transform;

void main()
{
	gl_Position = u_LightSpaceMatrix * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 410 core

void main()
{
}
