#include "gypch.h"
#include "Gymon/Renderer/PostProcess.h"

#include "Gymon/Renderer/RenderCommand.h"
#include "Gymon/Renderer/Texture.h"

#include <glad/glad.h>

namespace Gymon {

	namespace {

		// Bloom runs at half the scene's resolution; see the class comment.
		uint32_t Half(uint32_t value)
		{
			return value / 2 < 1 ? 1 : value / 2;
		}
	}

	PostProcess::PostProcess(uint32_t width, uint32_t height,
		const Ref<Shader>& bloomShader, const Ref<Shader>& compositeShader, uint32_t samples)
		: m_Width(width), m_Height(height), m_Samples(samples),
		  m_BloomShader(bloomShader), m_CompositeShader(compositeShader)
	{
		Invalidate();
	}

	void PostProcess::Invalidate()
	{
		FramebufferSpecification sceneSpec;
		sceneSpec.Width = m_Width;
		sceneSpec.Height = m_Height;
		sceneSpec.Samples = m_Samples;
		sceneSpec.HDR = true;
		m_Scene = Framebuffer::Create(sceneSpec);

		FramebufferSpecification bloomSpec;
		bloomSpec.Width = Half(m_Width);
		bloomSpec.Height = Half(m_Height);
		bloomSpec.HDR = true;
		m_BloomA = Framebuffer::Create(bloomSpec);
		m_BloomB = Framebuffer::Create(bloomSpec);

		// 8-bit: this is the display-referred result, and there is nothing
		// downstream of it that could use the extra range.
		FramebufferSpecification outputSpec;
		outputSpec.Width = m_Width;
		outputSpec.Height = m_Height;
		m_Output = Framebuffer::Create(outputSpec);
	}

	void PostProcess::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || (width == m_Width && height == m_Height))
			return;

		m_Width = width;
		m_Height = height;

		m_Scene->Resize(width, height);
		m_BloomA->Resize(Half(width), Half(height));
		m_BloomB->Resize(Half(width), Half(height));
		m_Output->Resize(width, height);
	}

	void PostProcess::Render()
	{
		// Every pass here is a fullscreen triangle over an existing image, so
		// depth testing would only ever reject work that must not be rejected.
		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetBlend(false);

		const uint32_t sceneTexture = m_Scene->GetColorAttachmentRendererID();

		if (BloomEnabled)
		{
			m_BloomShader->Bind();
			m_BloomShader->SetInt("u_Source", 0);
			m_BloomShader->SetFloat("u_Threshold", BloomThreshold);

			// Threshold the scene into the first bloom buffer.
			m_BloomA->Bind();
			RenderCommand::Clear();
			m_BloomShader->SetInt("u_Mode", 0);
			m_BloomShader->SetFloat("u_Radius", 1.0f);
			glBindTextureUnit(0, sceneTexture);
			RenderCommand::DrawArrays(3);
			m_BloomA->Unbind();

			// Ping-pong horizontal and vertical blurs. The radius grows with
			// each pair so a handful of nine-tap passes reaches a wide glow
			// without needing a wide kernel.
			for (int pass = 0; pass < BlurPasses; pass++)
			{
				const float radius = 1.0f + (float)pass * 1.6f;

				m_BloomB->Bind();
				RenderCommand::Clear();
				m_BloomShader->SetInt("u_Mode", 1);
				m_BloomShader->SetFloat("u_Radius", radius);
				glBindTextureUnit(0, m_BloomA->GetColorAttachmentRendererID());
				RenderCommand::DrawArrays(3);
				m_BloomB->Unbind();

				m_BloomA->Bind();
				RenderCommand::Clear();
				m_BloomShader->SetInt("u_Mode", 2);
				m_BloomShader->SetFloat("u_Radius", radius);
				glBindTextureUnit(0, m_BloomB->GetColorAttachmentRendererID());
				RenderCommand::DrawArrays(3);
				m_BloomA->Unbind();
			}
		}

		m_Output->Bind();
		RenderCommand::Clear();

		m_CompositeShader->Bind();
		m_CompositeShader->SetInt("u_Scene", 0);
		m_CompositeShader->SetInt("u_Bloom", 1);
		m_CompositeShader->SetFloat("u_Exposure", Exposure);
		m_CompositeShader->SetFloat("u_BloomIntensity", BloomEnabled ? BloomIntensity : 0.0f);

		glBindTextureUnit(0, sceneTexture);
		glBindTextureUnit(1, m_BloomA->GetColorAttachmentRendererID());

		RenderCommand::DrawArrays(3);
		m_Output->Unbind();

		RenderCommand::SetDepthWrite(true);
		RenderCommand::SetDepthTest(true);
	}
}
