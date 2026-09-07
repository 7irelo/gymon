#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Framebuffer.h"
#include "Gymon/Renderer/Shader.h"

namespace Gymon {

	// The tail of the frame: bloom, exposure, tonemap, gamma.
	//
	// Owns three targets. The scene is drawn into an HDR, multisampled buffer;
	// bright areas are extracted and blurred at half resolution through a pair
	// of ping-pong buffers; the result is composited into an 8-bit buffer that
	// the editor displays.
	//
	// Half resolution for the blur is not a corner cut. Bloom is a very wide,
	// very smooth signal, so a half-resolution blur is visually identical to a
	// full-resolution one and costs a quarter as much -- and the free bilinear
	// upsample on the way back widens the kernel for nothing.
	class PostProcess
	{
	public:
		// The shaders are passed in rather than loaded here so the engine does
		// not have to assume where an application keeps its assets.
		PostProcess(uint32_t width, uint32_t height,
			const Ref<Shader>& bloomShader, const Ref<Shader>& compositeShader,
			uint32_t samples = 4);

		void Resize(uint32_t width, uint32_t height);

		// The HDR target to render the scene into. Bind it, draw, unbind.
		const Ref<Framebuffer>& GetSceneTarget() const { return m_Scene; }

		// Runs the bloom chain and composite. Leaves the result in the output
		// target, which is what GetOutputTexture returns.
		void Render();

		// The display-ready texture, for ImGui::Image.
		uint32_t GetOutputTexture() const { return m_Output->GetColorAttachmentRendererID(); }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		// Tuning, surfaced so the editor can expose it.
		float Exposure = 1.0f;
		float BloomThreshold = 1.15f;
		float BloomIntensity = 0.06f;
		bool BloomEnabled = true;

		// How many horizontal/vertical blur pairs to run. More is wider and
		// smoother; four is enough for a glow that does not look like a box.
		int BlurPasses = 4;

	private:
		void Invalidate();

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_Samples = 1;

		Ref<Shader> m_BloomShader;
		Ref<Shader> m_CompositeShader;

		Ref<Framebuffer> m_Scene;
		Ref<Framebuffer> m_BloomA;
		Ref<Framebuffer> m_BloomB;
		Ref<Framebuffer> m_Output;
	};
}
