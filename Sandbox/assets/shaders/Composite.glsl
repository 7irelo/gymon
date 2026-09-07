// Final composite: bloom, exposure, tonemap, gamma.
//
// This is the only place in the pipeline that converts linear HDR light into
// display-referred sRGB. Everything upstream -- the sky, the PBR surfaces, the
// grid -- writes unbounded linear values into a float target, which is what
// makes it correct to blur, add and tonemap them here rather than each pass
// guessing at a display encoding of its own.

#type vertex
#version 410 core

out vec2 v_UV;

void main()
{
	vec2 positions[3] = vec2[3](
		vec2(-1.0, -1.0),
		vec2( 3.0, -1.0),
		vec2(-1.0,  3.0)
	);

	vec2 ndc = positions[gl_VertexID];
	v_UV = ndc * 0.5 + 0.5;
	gl_Position = vec4(ndc, 0.0, 1.0);
}

#type fragment
#version 410 core

layout(location = 0) out vec4 color;

in vec2 v_UV;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;

uniform float u_Exposure = 1.0;
uniform float u_BloomIntensity = 0.06;

// Fraction of the frame radius at which vignetting starts, and how dark the
// corners get. Subtle on purpose: a heavy vignette reads as a filter, a light
// one reads as a lens.
uniform float u_VignetteStrength = 0.22;

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 scene = texture(u_Scene, v_UV).rgb;
	vec3 bloom = texture(u_Bloom, v_UV).rgb;

	// Added rather than mixed: bloom is light that scattered off the intended
	// path inside the lens, so it is extra energy on top of the image, not a
	// blend with it.
	vec3 hdr = (scene + bloom * u_BloomIntensity) * u_Exposure;

	vec2 centred = v_UV - 0.5;
	float vignette = 1.0 - u_VignetteStrength * dot(centred, centred) * 2.0;
	hdr *= vignette;

	color = vec4(pow(ACESFilm(hdr), vec3(1.0 / 2.2)), 1.0);
}
