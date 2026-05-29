#include "Sandbox2D.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::OnAttach()
{
	// Try to load a texture if present; otherwise the white texture path still works.
	m_CheckerboardTexture = Gymon::Texture2D::Create("assets/textures/Checkerboard.png");
}

void Sandbox2D::OnDetach()
{
}

void Sandbox2D::OnUpdate(Gymon::Timestep ts)
{
	if (!m_Active)
		return;

	// Update
	m_CameraController.OnUpdate(ts);
	m_Rotation += ts * 50.0f;

	// Render
	Gymon::Renderer2D::ResetStats();
	Gymon::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
	Gymon::RenderCommand::Clear();

	Gymon::Renderer2D::BeginScene(m_CameraController.GetCamera());

	Gymon::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
	Gymon::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, m_SquareColor);
	Gymon::Renderer2D::DrawRotatedQuad({ 1.0f, 0.5f }, { 0.6f, 0.6f }, m_Rotation, { 0.2f, 0.8f, 0.3f, 1.0f });

	if (m_CheckerboardTexture && m_CheckerboardTexture->IsLoaded())
		Gymon::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f }, m_CheckerboardTexture, 10.0f);

	// A grid of quads to exercise batching
	for (float y = -5.0f; y < 5.0f; y += 0.5f)
	{
		for (float x = -5.0f; x < 5.0f; x += 0.5f)
		{
			glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f };
			Gymon::Renderer2D::DrawQuad({ x, y }, { 0.45f, 0.45f }, color);
		}
	}

	Gymon::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender()
{
	if (!m_Active)
		return;

	ImGui::Begin("Settings");

	auto stats = Gymon::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	ImGui::End();
}

void Sandbox2D::OnEvent(Gymon::Event& e)
{
	if (!m_Active)
		return;

	m_CameraController.OnEvent(e);
}
