#include "gypch.h"
#include "Gymon/Renderer/ShadowMap.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gymon {

	ShadowMap::ShadowMap(uint32_t resolution)
		: m_Resolution(resolution)
	{
		glCreateFramebuffers(1, &m_RendererID);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
		glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Resolution, m_Resolution,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clamp to a border of 1.0 (maximum depth) so anything sampled outside
		// the light frustum reads as fully lit. The alternative, repeat, tiles
		// the shadow map and paints phantom shadows across the whole scene.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

		glNamedFramebufferTexture(m_RendererID, GL_DEPTH_ATTACHMENT, m_DepthAttachment, 0);

		// A framebuffer with no colour attachment is incomplete unless it is
		// explicitly told not to expect one.
		glNamedFramebufferDrawBuffer(m_RendererID, GL_NONE);
		glNamedFramebufferReadBuffer(m_RendererID, GL_NONE);

		GY_CORE_ASSERT(glCheckNamedFramebufferStatus(m_RendererID, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
			"Shadow map framebuffer is incomplete!");
	}

	ShadowMap::~ShadowMap()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures(1, &m_DepthAttachment);
	}

	void ShadowMap::SetLight(const glm::vec3& direction, const glm::vec3& sceneCenter, float sceneRadius)
	{
		const glm::vec3 lightDir = glm::normalize(direction);

		// Pull the virtual eye back along the light by more than the radius so
		// the whole scene sits in front of the near plane.
		const float distance = sceneRadius * 2.0f + 1.0f;
		const glm::vec3 eye = sceneCenter - lightDir * distance;

		// A light pointing straight down is parallel to the usual world up, so
		// glm::lookAt would produce a degenerate basis. Swap the up vector in
		// that case rather than emitting NaNs into the matrix.
		const glm::vec3 up = std::abs(lightDir.y) > 0.99f
			? glm::vec3(0.0f, 0.0f, 1.0f)
			: glm::vec3(0.0f, 1.0f, 0.0f);

		const glm::mat4 view = glm::lookAt(eye, sceneCenter, up);

		// Orthographic, because a directional light's rays are parallel.
		const glm::mat4 projection = glm::ortho(
			-sceneRadius, sceneRadius,
			-sceneRadius, sceneRadius,
			0.1f, distance + sceneRadius);

		m_LightSpaceMatrix = projection * view;
	}

	void ShadowMap::BeginPass()
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PreviousFramebuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Resolution, m_Resolution);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Front-face culling during the depth pass pushes acne onto surfaces
		// facing away from the camera, where it is not visible. Cheaper and
		// more robust than tuning a depth bias alone.
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	void ShadowMap::EndPass(uint32_t restoreWidth, uint32_t restoreHeight)
	{
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);

		glBindFramebuffer(GL_FRAMEBUFFER, (uint32_t)m_PreviousFramebuffer);
		glViewport(0, 0, restoreWidth, restoreHeight);
	}

	void ShadowMap::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_DepthAttachment);
	}
}
