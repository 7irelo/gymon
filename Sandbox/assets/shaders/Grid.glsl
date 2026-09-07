// Infinite editor ground grid.
//
// Drawn as a fullscreen pass rather than as a large quad of geometry: each
// pixel intersects its view ray with the y = 0 plane and shades the result.
// A quad would have to be finite (so the grid visibly ends), would need
// enough tessellation to avoid depth precision artefacts, and would still
// alias badly in the distance.
//
// Because the intersection gives a real world position, the pass writes
// gl_FragDepth, so the grid is occluded by scene geometry correctly and does
// not need to be drawn first or last for the wrong reasons.

#type vertex
#version 410 core

out vec2 v_NDC;

void main()
{
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

uniform mat4 u_ViewProjection;
uniform mat4 u_InverseViewProjection;
uniform vec3 u_CameraPosition;

// Spacing of the fine lines, in world units. Every tenth line is drawn
// brighter, the way a ruler marks centimetres inside decimetres.
uniform float u_CellSize = 1.0;
uniform float u_FadeDistance = 120.0;

// Anti-aliased grid coverage at one spacing, using screen-space derivatives so
// distant lines fade out instead of turning into aliased noise.
float GridCoverage(vec2 position, float spacing)
{
	vec2 coord = position / spacing;
	vec2 derivative = fwidth(coord);

	// Distance to the nearest line, measured in pixels.
	vec2 grid = abs(fract(coord - 0.5) - 0.5) / max(derivative, vec2(1e-5));

	return 1.0 - min(min(grid.x, grid.y), 1.0);
}

void main()
{
	vec4 nearPoint = u_InverseViewProjection * vec4(v_NDC, -1.0, 1.0);
	vec4 farPoint  = u_InverseViewProjection * vec4(v_NDC,  1.0, 1.0);
	vec3 origin = nearPoint.xyz / nearPoint.w;
	vec3 dir = normalize(farPoint.xyz / farPoint.w - origin);

	// Rays that do not descend towards the plane never hit it. The epsilon
	// also kills the near-horizon case, where t explodes and the grid would
	// smear into a bright band.
	if (abs(dir.y) < 1e-4)
		discard;

	float t = -origin.y / dir.y;
	if (t <= 0.0)
		discard;

	vec3 hit = origin + dir * t;

	float fine = GridCoverage(hit.xz, u_CellSize);
	float coarse = GridCoverage(hit.xz, u_CellSize * 10.0);

	// Distance fade, so the grid dissolves into the sky rather than ending at
	// a hard circle or aliasing into moire.
	float distanceFade = 1.0 - smoothstep(u_FadeDistance * 0.35, u_FadeDistance, length(hit - u_CameraPosition));
	if (distanceFade <= 0.0)
		discard;

	// Linear, because the target is now a linear HDR buffer: an sRGB-looking
	// 0.3 would come out of the tonemapper considerably brighter than intended.
	vec3 lineColor = vec3(0.075);
	float alpha = fine * 0.28 + coarse * 0.45;

	// World axes, coloured the way every DCC tool colours them: X red, Z blue.
	// Tested against the line width at this distance so the axis stays one
	// line thick no matter how far away it is.
	vec2 axisWidth = fwidth(hit.xz) * 1.2;
	if (abs(hit.z) < axisWidth.y)
	{
		lineColor = vec3(0.52, 0.04, 0.05);
		alpha = max(alpha, 0.85);
	}
	else if (abs(hit.x) < axisWidth.x)
	{
		lineColor = vec3(0.04, 0.14, 0.68);
		alpha = max(alpha, 0.85);
	}

	alpha *= distanceFade;
	if (alpha <= 0.001)
		discard;

	// Depth from the actual intersection, so scene geometry in front of the
	// plane occludes the grid.
	vec4 clip = u_ViewProjection * vec4(hit, 1.0);
	gl_FragDepth = (clip.z / clip.w) * 0.5 + 0.5;

	color = vec4(lineColor, alpha);
}
