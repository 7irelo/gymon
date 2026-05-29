#include "gypch.h"
#include "Gymon/Renderer/RenderCommand.h"

namespace Gymon {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}
