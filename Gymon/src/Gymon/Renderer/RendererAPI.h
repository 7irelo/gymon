#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace Gymon {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void SetDepthTest(bool enabled) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

		// Non-indexed draw with no vertex data at all: the vertex shader
		// derives its positions from gl_VertexID. Used for fullscreen passes
		// (sky, grid), where uploading three vertices to describe the whole
		// screen is pure ceremony. Core profile still requires *some* vertex
		// array to be bound, hence the empty one.
		virtual void DrawArrays(uint32_t vertexCount) = 0;

		// Depth writes are separate from the depth test: the sky needs the
		// test off and the write off, the grid needs the test on and the
		// write off so it never occludes geometry behind it.
		virtual void SetDepthWrite(bool enabled) = 0;
		virtual void SetBlend(bool enabled) = 0;

		static API GetAPI() { return s_API; }
		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};
}
