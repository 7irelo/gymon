#pragma once

#include "Gymon/Core.h"

#include <glm/glm.hpp>

namespace Gymon {

	// A depth-only render target for directional shadow mapping.
	//
	// The scene is rendered once from the light's point of view into a depth
	// buffer; the main pass then reprojects each fragment into that space and
	// compares depths to decide whether it is occluded.
	//
	// Kept separate from Framebuffer because it has no colour attachment at
	// all, and because the light-space matrix belongs with the map that was
	// rendered using it -- passing them around independently is how shadow
	// bugs happen.
	class ShadowMap
	{
	public:
		// Square maps only. 2048 is a reasonable default: sharp enough for a
		// scene tens of units across, cheap enough to re-render every frame.
		explicit ShadowMap(uint32_t resolution = 2048);
		~ShadowMap();

		ShadowMap(const ShadowMap&) = delete;
		ShadowMap& operator=(const ShadowMap&) = delete;

		// Binds the depth target and sets the viewport to the map resolution.
		void BeginPass();
		// Restores the framebuffer that was bound at BeginPass, and the given
		// viewport. The previous binding is remembered rather than assumed to
		// be the default: in the editor the scene renders into the viewport
		// framebuffer, so binding 0 here would send the frame to the window.
		void EndPass(uint32_t restoreWidth, uint32_t restoreHeight);

		// Fits an orthographic light frustum around a sphere covering the
		// scene. Directional light has no position, so the "eye" is placed
		// back along the light direction far enough to enclose everything.
		void SetLight(const glm::vec3& direction, const glm::vec3& sceneCenter, float sceneRadius);

		const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }

		uint32_t GetDepthAttachmentRendererID() const { return m_DepthAttachment; }
		uint32_t GetResolution() const { return m_Resolution; }

		void Bind(uint32_t slot) const;

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_DepthAttachment = 0;
		uint32_t m_Resolution = 0;
		int m_PreviousFramebuffer = 0;
		glm::mat4 m_LightSpaceMatrix{ 1.0f };
	};
}
