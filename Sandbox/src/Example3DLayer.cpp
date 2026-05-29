#include "Example3DLayer.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const std::string s_LightingVertex = R"(
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform mat3 u_NormalMatrix;

out vec3 v_FragPos;
out vec3 v_Normal;

void main()
{
	vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
	v_FragPos = worldPos.xyz;
	v_Normal = u_NormalMatrix * a_Normal;
	gl_Position = u_ViewProjection * worldPos;
}
)";

static const std::string s_LightingFragment = R"(
#version 410 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;

uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform vec3 u_ObjectColor;
uniform vec3 u_ViewPos;

void main()
{
	// Ambient
	float ambientStrength = 0.15;
	vec3 ambient = ambientStrength * u_LightColor;

	// Diffuse
	vec3 norm = normalize(v_Normal);
	vec3 lightDir = normalize(u_LightPos - v_FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * u_LightColor;

	// Specular
	float specularStrength = 0.5;
	vec3 viewDir = normalize(u_ViewPos - v_FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
	vec3 specular = specularStrength * spec * u_LightColor;

	vec3 result = (ambient + diffuse + specular) * u_ObjectColor;
	color = vec4(result, 1.0);
}
)";

Example3DLayer::Example3DLayer()
	: Layer("Example3DLayer"), m_CameraController(1280.0f / 720.0f, 45.0f)
{
}

void Example3DLayer::OnAttach()
{
	m_CubeMesh = Gymon::Mesh::CreateCube();
	m_LightingShader = Gymon::Shader::Create("Lighting", s_LightingVertex, s_LightingFragment);
}

void Example3DLayer::OnDetach()
{
}

void Example3DLayer::OnUpdate(Gymon::Timestep ts)
{
	if (!m_Active)
		return;

	m_CameraController.OnUpdate(ts);
	m_Angle += ts * 40.0f;

	Gymon::RenderCommand::SetDepthTest(true);
	Gymon::RenderCommand::SetClearColor({ 0.05f, 0.05f, 0.08f, 1.0f });
	Gymon::RenderCommand::Clear();

	auto& camera = m_CameraController.GetCamera();
	Gymon::Renderer::BeginScene(camera);

	m_LightingShader->Bind();
	m_LightingShader->SetFloat3("u_LightPos", m_LightPosition);
	m_LightingShader->SetFloat3("u_LightColor", m_LightColor);
	m_LightingShader->SetFloat3("u_ViewPos", camera.GetPosition());

	// A 3x3 grid of rotating cubes.
	for (int x = -1; x <= 1; x++)
	{
		for (int z = -1; z <= 1; z++)
		{
			glm::vec3 pos = { x * 2.0f, 0.0f, z * 2.0f };
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos)
				* glm::rotate(glm::mat4(1.0f), glm::radians(m_Angle + (x + z) * 20.0f), glm::vec3(0.4f, 1.0f, 0.2f));

			glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));
			m_LightingShader->SetMat3("u_NormalMatrix", normalMatrix);
			m_LightingShader->SetFloat3("u_ObjectColor", m_ObjectColor);

			Gymon::Renderer::Submit(m_LightingShader, m_CubeMesh->GetVertexArray(), transform);
		}
	}

	// Visualize the light source as a small bright cube.
	{
		glm::mat4 lightTransform = glm::translate(glm::mat4(1.0f), m_LightPosition)
			* glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(lightTransform)));
		m_LightingShader->SetMat3("u_NormalMatrix", normalMatrix);
		m_LightingShader->SetFloat3("u_ObjectColor", m_LightColor);
		Gymon::Renderer::Submit(m_LightingShader, m_CubeMesh->GetVertexArray(), lightTransform);
	}

	Gymon::Renderer::EndScene();
}

void Example3DLayer::OnImGuiRender()
{
	if (!m_Active)
		return;

	ImGui::Begin("3D Scene");
	ImGui::Text("WASD to move, hold RMB to look, scroll to zoom");
	ImGui::DragFloat3("Light Position", glm::value_ptr(m_LightPosition), 0.1f);
	ImGui::ColorEdit3("Light Color", glm::value_ptr(m_LightColor));
	ImGui::ColorEdit3("Object Color", glm::value_ptr(m_ObjectColor));
	ImGui::End();
}

void Example3DLayer::OnEvent(Gymon::Event& e)
{
	if (!m_Active)
		return;

	m_CameraController.OnEvent(e);
}
