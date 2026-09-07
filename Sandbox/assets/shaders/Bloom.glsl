// Bloom: bright extraction and separable Gaussian blur.
//
// One shader with a mode switch rather than three files, because the three
// passes differ by about six lines and keeping them together makes the chain
// readable in one place.
//
//   mode 0 - threshold: keep only what is brighter than the knee
//   mode 1 - blur horizontally
//   mode 2 - blur vertically
//
// The blur is separable: a 2D Gaussian is the product of two 1D Gaussians, so
// nine taps twice costs eighteen samples instead of eighty-one.

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

uniform sampler2D u_Source;
uniform int u_Mode;

// Luminance above which a pixel contributes to the bloom, and the width of
// the soft transition below it. A hard cutoff makes bloom pop in and out as
// a highlight drifts across the threshold.
uniform float u_Threshold = 1.15;
uniform float u_Knee = 0.55;

// Multiplies the blur step, so the same nine taps can cover a wider radius on
// a later, lower-resolution pass.
uniform float u_Radius = 1.0;

const float kWeights[5] = float[5](0.227027, 0.194594, 0.121621, 0.054054, 0.016216);

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(u_Source, 0));

	if (u_Mode == 0)
	{
		// Non-finite input is discarded rather than propagated: the blur that
		// follows would smear one bad pixel across the whole neighbourhood.
		vec3 source = texture(u_Source, v_UV).rgb;
		if (any(isnan(source)) || any(isinf(source)))
			source = vec3(0.0);

		// Rec. 709 luminance: the eye is far more sensitive to green than to
		// blue, and thresholding on the raw maximum channel makes saturated
		// blues bloom long before they look bright.
		float luminance = dot(source, vec3(0.2126, 0.7152, 0.0722));

		// Soft knee, quadratic through the transition band.
		float soft = clamp(luminance - u_Threshold + u_Knee, 0.0, 2.0 * u_Knee);
		soft = soft * soft / (4.0 * u_Knee + 1e-5);

		float contribution = max(soft, luminance - u_Threshold) / max(luminance, 1e-5);

		color = vec4(source * contribution, 1.0);
		return;
	}

	vec2 direction = u_Mode == 1 ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
	direction *= u_Radius;

	vec3 result = texture(u_Source, v_UV).rgb * kWeights[0];
	for (int i = 1; i < 5; i++)
	{
		result += texture(u_Source, v_UV + direction * float(i)).rgb * kWeights[i];
		result += texture(u_Source, v_UV - direction * float(i)).rgb * kWeights[i];
	}

	color = vec4(result, 1.0);
}
