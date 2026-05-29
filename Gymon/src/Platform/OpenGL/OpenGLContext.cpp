#include "gypch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Gymon {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		GY_CORE_ASSERT(windowHandle, "Window handle is null!");
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		GY_CORE_ASSERT(status, "Failed to initialize Glad!");

		GY_CORE_INFO("OpenGL Info:");
		GY_CORE_INFO("  Vendor:   {0}", (const char*)glGetString(GL_VENDOR));
		GY_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		GY_CORE_INFO("  Version:  {0}", (const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
	}
}
