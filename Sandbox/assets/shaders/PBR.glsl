// Physically based shading, metallic-roughness workflow.
//
// This replaces the Blinn-Phong shader for anything that wants to look modern.
// The three things that actually change how it reads, in order of impact:
//
//   1. All lighting is done in LINEAR space. Textures are sampled from sRGB
//      storage (the GPU linearises them), lighting is computed linearly, and
//      the result is converted back to sRGB at the very end. Doing this wrong
//      is the single most common reason a renderer looks flat and washed out.
//   2. Cook-Torrance specular with GGX distribution, Smith geometry and
//      Fresnel-Schlick, so highlights vary with roughness the way real
//      surfaces do instead of being a fixed power.
//   3. ACES filmic tonemapping, which maps unbounded HDR lighting into
//      displayable range without the highlight clipping that makes naive
//      renderers look plasticky.

#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
	vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
	v_WorldPos = worldPos.xyz;

	// Inverse-transpose so non-uniform scale does not skew the normal.
	v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;

	v_TexCoord = a_TexCoord;
	gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 410 core

layout(location = 0) out vec4 color;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPosition;

uniform vec4  u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform int u_HasAlbedoMap;
uniform int u_HasNormalMap;
uniform int u_HasMetallicRoughnessMap;

const float PI = 3.14159265359;

// GGX / Trowbridge-Reitz. Describes what fraction of microfacets are aligned
// with the halfway vector; the long tail is why rough metal has a wide,
// gradually fading highlight rather than a hard dot.
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * denom * denom, 1e-6);
}

// Smith geometry term: how much microfacets shadow and mask each other.
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness)
	     * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

// Fresnel: everything becomes a mirror at grazing angles.
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Derives a tangent basis from screen-space derivatives, so normal mapping
// works without the mesh carrying tangent vectors.
vec3 ApplyNormalMap(vec3 N, vec3 worldPos, vec2 uv)
{
	vec3 tangentNormal = texture(u_NormalMap, uv).xyz * 2.0 - 1.0;

	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(uv);
	vec2 st2 = dFdy(uv);

	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	return normalize(mat3(T, B, N) * tangentNormal);
}

// ACES filmic curve, Narkowicz's fit. Cheap, and close enough to the real
// thing that highlights roll off instead of clipping to white.
vec3 ACESFilm(vec3 x)
{
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	// Albedo maps are sRGB textures, so this fetch is already linear.
	vec4 albedoSample = u_HasAlbedoMap == 1 ? texture(u_AlbedoMap, v_TexCoord) : vec4(1.0);
	vec3 albedo = u_Albedo.rgb * albedoSample.rgb;
	float alpha = u_Albedo.a * albedoSample.a;

	float metallic = u_Metallic;
	float roughness = u_Roughness;
	if (u_HasMetallicRoughnessMap == 1)
	{
		// glTF packs roughness in G and metallic in B.
		vec3 mr = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
		roughness *= mr.g;
		metallic *= mr.b;
	}
	// Perfectly smooth surfaces produce a singular highlight, so clamp away
	// from zero rather than letting the distribution blow up.
	roughness = clamp(roughness, 0.04, 1.0);

	vec3 N = normalize(v_Normal);
	if (u_HasNormalMap == 1)
		N = ApplyNormalMap(N, v_WorldPos, v_TexCoord);

	vec3 V = normalize(u_ViewPosition - v_WorldPos);
	vec3 L = normalize(-u_LightDirection);
	vec3 H = normalize(V + L);

	// Dielectrics reflect ~4% head-on; metals reflect their albedo instead of
	// diffusing it, which is the whole distinction the metallic term encodes.
	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
	vec3 specular = numerator / denominator;

	// Energy conservation: light reflected specularly is not also diffused,
	// and metals have no diffuse response at all.
	vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

	float NdotL = max(dot(N, L), 0.0);
	vec3 Lo = (kD * albedo / PI + specular) * u_LightColor * NdotL;

	// Stand-in for image-based lighting until an environment map exists.
	// A flat term keeps shadowed faces from going pure black.
	vec3 ambient = vec3(0.03) * albedo;

	vec3 hdr = ambient + Lo;
	vec3 mapped = ACESFilm(hdr);

	// Back to sRGB for display. The framebuffer is a plain RGBA8 target, so
	// this has to be done explicitly rather than relying on GL_FRAMEBUFFER_SRGB.
	color = vec4(pow(mapped, vec3(1.0 / 2.2)), alpha);
}
