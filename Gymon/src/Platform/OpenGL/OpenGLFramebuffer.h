#pragma once

#include "Gymon/Renderer/Framebuffer.h"

namespace Gymon {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		explicit OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		// Recreates the FBO and its attachments. Called by the constructor and
		// on every resize, since attachment storage is immutable once created.
		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetColorAttachmentRendererID() const override { return m_ResolveColor; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		void Release();
		bool IsMultisampled() const { return m_Specification.Samples > 1; }

		// The target actually rendered into. Multisampled when Samples > 1.
		uint32_t m_RendererID = 0;
		uint32_t m_ColorAttachment = 0;
		uint32_t m_DepthAttachment = 0;

		// The single-sample target Unbind() resolves into. When the
		// framebuffer is not multisampled there is no second target: the
		// colour attachment is already sampleable and m_ResolveColor aliases
		// it, so the resolve is skipped entirely.
		uint32_t m_ResolveFBO = 0;
		uint32_t m_ResolveColor = 0;

		FramebufferSpecification m_Specification;
	};
}
