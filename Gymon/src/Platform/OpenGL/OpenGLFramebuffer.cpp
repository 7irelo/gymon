#include "gypch.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

#include <glad/glad.h>

namespace Gymon {

	// A resize to something absurd is almost always a transient value from a
	// docked panel mid-layout, so clamp rather than trying to allocate it.
	static constexpr uint32_t s_MaxFramebufferSize = 8192;

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		Invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		Release();
	}

	void OpenGLFramebuffer::Release()
	{
		if (m_RendererID)
			glDeleteFramebuffers(1, &m_RendererID);
		if (m_ColorAttachment)
			glDeleteTextures(1, &m_ColorAttachment);
		if (m_DepthAttachment)
			glDeleteTextures(1, &m_DepthAttachment);

		// Only a separate object when multisampling; otherwise m_ResolveColor
		// aliases m_ColorAttachment and must not be deleted twice.
		if (m_ResolveFBO)
		{
			glDeleteFramebuffers(1, &m_ResolveFBO);
			glDeleteTextures(1, &m_ResolveColor);
		}

		m_RendererID = 0;
		m_ColorAttachment = 0;
		m_DepthAttachment = 0;
		m_ResolveFBO = 0;
		m_ResolveColor = 0;
	}

	void OpenGLFramebuffer::Invalidate()
	{
		Release();

		const GLenum colorFormat = m_Specification.HDR ? GL_RGBA16F : GL_RGBA8;
		const GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		glCreateTextures(target, 1, &m_ColorAttachment);
		glBindTexture(target, m_ColorAttachment);

		if (IsMultisampled())
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples,
				colorFormat, m_Specification.Width, m_Specification.Height, GL_TRUE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, m_Specification.Width, m_Specification.Height,
				0, GL_RGBA, m_Specification.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Clamped so that a viewport edge does not sample the opposite side.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_ColorAttachment, 0);

		// Combined depth/stencil: 3D needs depth, and stencil costs nothing
		// extra in the packed format.
		glCreateTextures(target, 1, &m_DepthAttachment);
		glBindTexture(target, m_DepthAttachment);

		if (IsMultisampled())
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples,
				GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height, GL_TRUE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8,
				m_Specification.Width, m_Specification.Height);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, m_DepthAttachment, 0);

		GY_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
			"Framebuffer is incomplete!");

		if (IsMultisampled())
		{
			// The destination of the resolve blit. Single-sample, same format,
			// and the only one of the two that anything can sample.
			glCreateFramebuffers(1, &m_ResolveFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBO);

			glCreateTextures(GL_TEXTURE_2D, 1, &m_ResolveColor);
			glBindTexture(GL_TEXTURE_2D, m_ResolveColor);
			glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, m_Specification.Width, m_Specification.Height,
				0, GL_RGBA, m_Specification.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolveColor, 0);

			GY_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
				"Resolve framebuffer is incomplete!");
		}
		else
		{
			m_ResolveColor = m_ColorAttachment;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Specification.Width, m_Specification.Height);
	}

	void OpenGLFramebuffer::Unbind()
	{
		if (IsMultisampled())
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFBO);
			glBlitFramebuffer(
				0, 0, m_Specification.Width, m_Specification.Height,
				0, 0, m_Specification.Width, m_Specification.Height,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			GY_CORE_WARN("Ignoring framebuffer resize to {0}x{1}", width, height);
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;
		Invalidate();
	}

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		return CreateRef<OpenGLFramebuffer>(spec);
	}
}
