#include "gypch.h"
#include "Gymon/Renderer/Renderer.h"
#include "Gymon/Renderer/Renderer2D.h"

#include "Platform/OpenGL/OpenGLShader.h"

namespace Gymon {

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();
	static Renderer::Statistics s_Stats;

	void Renderer::Init()
	{
		RenderCommand::Init();
		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::BeginScene(PerspectiveCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform, uint32_t indexCount)
	{
		shader->Bind();
		std::static_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		std::static_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray, indexCount);

		const uint32_t drawn = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		s_Stats.DrawCalls++;
		s_Stats.MeshCount++;
		s_Stats.TriangleCount += drawn / 3;
		s_Stats.VertexCount += drawn;
	}

	void Renderer::ResetStats()
	{
		s_Stats = Statistics{};
	}

	Renderer::Statistics Renderer::GetStats()
	{
		return s_Stats;
	}
}
