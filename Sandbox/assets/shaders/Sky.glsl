// Procedural sky, drawn as a single fullscreen triangle before the scene.
//
// This is an analytic approximation rather than a real atmospheric scattering
// integral: a zenith-to-horizon gradient, a warm haze band at the horizon, a
// sun disc with a wide glow, and a ground half. That is enough to give the
// scene a horizon, a colour that changes with view direction, and a light
// source you can actually see -- the three things whose absence makes a
// renderer read as "objects floating in a void".
//
// It is also what feeds the hemispheric ambient in PBR.glsl, so sky colour and
// the ambient tint on surfaces are deliberately in the same family.

#type vertex
#version 410 core

out vec2 v_NDC;

void main()
{
	// A single oversized triangle covering the screen, generated from the
	// vertex index. Cheaper than a quad (no diagonal seam, no vertex buffer)
	// and the standard way to do a fullscreen pass.
	vec2 positions[3] = vec2[3](
		vec2(-1.0, -1.0),
		vec2( 3.0, -1.0),
		vec2(-1.0,  3.0)
	);

	v_NDC = positions[gl_VertexID];
	gl_Position = vec4(v_NDC, 1.0, 1.0);
}

#type fragment
#version 410 core

layout(location = 0) out vec4 color;

in vec2 v_NDC;

uniform mat4 u_InverseViewProjection;
uniform vec3 u_CameraPosition;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;

uniform vec3 u_SkyColor    = vec3(0.20, 0.36, 0.68);
uniform vec3 u_HorizonColor = vec3(0.62, 0.71, 0.84);
uniform vec3 u_GroundColor = vec3(0.16, 0.14, 0.12);
uniform float u_Exposure = 1.0;

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	// Unproject two points on the same ray and subtract: this recovers the
	// world-space view direction for this pixel without needing the camera
	// basis vectors as separate uniforms.
	vec4 nearPoint = u_InverseViewProjection * vec4(v_NDC, -1.0, 1.0);
	vec4 farPoint  = u_InverseViewProjection * vec4(v_NDC,  1.0, 1.0);
	vec3 dir = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);

	vec3 sunDir = normalize(-u_LightDirection);

	// Height above the horizon, 0 at the horizon and 1 at the zenith. The
	// power curve compresses the gradient towards the horizon, where the real
	// thing changes fastest.
	float up = dir.y;
	float skyBlend = pow(clamp(up, 0.0, 1.0), 0.62);

	vec3 sky = mix(u_HorizonColor, u_SkyColor, skyBlend);

	// Sun. The tight power is the disc, the loose one the atmospheric glow
	// around it that sells the direction of the light.
	float sunAmount = max(dot(dir, sunDir), 0.0);
	vec3 sun = u_LightColor * (pow(sunAmount, 900.0) * 12.0 + pow(sunAmount, 24.0) * 0.35);

	// Below the horizon: a dark ground plane with the sky bleeding into it
	// near the horizon line, so the transition is not a hard edge.
	float belowBlend = smoothstep(0.0, -0.06, up);
	vec3 ground = mix(u_HorizonColor * 0.45, u_GroundColor, smoothstep(0.0, -0.35, up));

	vec3 hdr = mix(sky + sun, ground, belowBlend);

	color = vec4(pow(ACESFilm(hdr * u_Exposure), vec3(1.0 / 2.2)), 1.0);
}
