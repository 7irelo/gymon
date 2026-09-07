// Directional-light Blinn-Phong shader for the editor scene.
//
// Loaded from disk rather than embedded in the binary so that it can be
// hot-reloaded: save this file with the Sandbox running and the change is
// picked up on the next frame. A syntax error is logged and the previously
// compiled program is kept, so a bad edit will not black-screen the app.

#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
	vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
	v_FragPos = worldPos.xyz;

	// Derived here rather than passed in, so a mesh can be non-uniformly
	// scaled in the inspector without its lighting going wrong.
	v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;

	v_TexCoord = a_TexCoord;
	gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 410 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform vec3 u_ViewPosition;

uniform vec4 u_Color;
uniform float u_Shininess;

void main()
{
	vec3 normal = normalize(v_Normal);

	// u_LightDirection points the way the light travels, so the vector
	// towards the light is its negation.
	vec3 lightDir = normalize(-u_LightDirection);

	vec3 ambient = 0.15 * u_LightColor;

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * u_LightColor;

	// Blinn-Phong: halfway vector rather than the reflection vector, which
	// stays stable at grazing angles where Phong's specular breaks up.
	vec3 viewDir = normalize(u_ViewPosition - v_FragPos);
	vec3 halfway = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfway), 0.0), max(u_Shininess, 1.0));
	vec3 specular = 0.4 * spec * u_LightColor;

	vec3 result = (ambient + diffuse + specular) * u_Color.rgb;
	color = vec4(result, u_Color.a);
}
