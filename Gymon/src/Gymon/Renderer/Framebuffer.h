#pragma once

#include "Gymon/Core.h"

namespace Gymon {

	struct FramebufferSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;

		// Set when the framebuffer is presented to the screen rather than
		// sampled as a texture. The editor viewport wants the default (false).
		bool SwapChainTarget = false;
	};

	// An off-screen render target.
	//
	// The editor needs the scene as a texture so it can be drawn inside an
	// ImGui panel rather than over the whole window. Everything that wants
	// post-processing, shadow maps or picking needs the same thing.
	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		// Recreates the attachments at the new size. Cheap to guard against:
		// callers should only invoke it when the size actually changed.
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		// The colour attachment, for handing to ImGui::Image.
		virtual uint32_t GetColorAttachmentRendererID() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};
}
