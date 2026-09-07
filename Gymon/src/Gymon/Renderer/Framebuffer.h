#pragma once

#include "Gymon/Core.h"

namespace Gymon {

	struct FramebufferSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;

		// Multisample count. 1 is no multisampling.
		//
		// When this is greater than 1 the framebuffer keeps two sets of
		// attachments: a multisampled pair to render into, and a single-sample
		// colour texture that Unbind() resolves into, because a multisample
		// texture cannot be sampled by an ordinary shader or handed to ImGui.
		uint32_t Samples = 1;

		// Allocates the colour attachment as RGBA16F instead of RGBA8.
		//
		// Lighting produces values well above 1: a sun disc, a specular
		// highlight on a wet surface. An 8-bit target clips those on the way
		// in, before the tonemapper ever sees them, which is why a renderer
		// that tonemaps into an 8-bit buffer still looks clipped. It is also
		// what gives bloom something to bloom from.
		bool HDR = false;

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

		// Unbinds, first resolving multisampled colour into the sampleable
		// texture when the framebuffer is multisampled.
		virtual void Unbind() = 0;

		// Recreates the attachments at the new size. Cheap to guard against:
		// callers should only invoke it when the size actually changed.
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		// The sampleable colour attachment, for handing to ImGui::Image or to
		// a post-processing pass. Always single-sample.
		virtual uint32_t GetColorAttachmentRendererID() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};
}
