#pragma once

#include "Gymon/Renderer/RenderCommand.h"
#include "Gymon/Renderer/OrthographicCamera.h"
#include "Gymon/Renderer/PerspectiveCamera.h"
#include "Gymon/Renderer/Shader.h"

namespace Gymon {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void BeginScene(PerspectiveCamera& camera);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray,
			const glm::mat4& transform = glm::mat4(1.0f), uint32_t indexCount = 0);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		// Per-frame 3D counters. Renderer2D keeps its own separate batch stats;
		// these cover geometry submitted through Renderer::Submit.
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t MeshCount = 0;
			uint32_t TriangleCount = 0;
			uint32_t VertexCount = 0;
		};

		static void ResetStats();
		static Statistics GetStats();
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
	};
}
