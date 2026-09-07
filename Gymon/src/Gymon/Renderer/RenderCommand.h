#pragma once

#include "Gymon/Renderer/RendererAPI.h"

namespace Gymon {

	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		static void SetDepthTest(bool enabled)
		{
			s_RendererAPI->SetDepthTest(enabled);
		}

		static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawArrays(uint32_t vertexCount)
		{
			s_RendererAPI->DrawArrays(vertexCount);
		}

		static void SetDepthWrite(bool enabled)
		{
			s_RendererAPI->SetDepthWrite(enabled);
		}

		static void SetBlend(bool enabled)
		{
			s_RendererAPI->SetBlend(enabled);
		}
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};
}
