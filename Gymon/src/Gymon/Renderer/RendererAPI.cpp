#include "gypch.h"
#include "Gymon/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Gymon {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
			case RendererAPI::API::None:    GY_CORE_ASSERT(false, "RendererAPI::None is not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLRendererAPI>();
		}

		GY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
