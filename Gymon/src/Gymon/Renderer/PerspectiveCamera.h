#pragma once

#include <glm/glm.hpp>

namespace Gymon {

	// A simple free-look perspective camera positioned with yaw/pitch.
	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(float fov = 45.0f, float aspectRatio = 1.778f, float nearClip = 0.1f, float farClip = 1000.0f);

		void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateView(); }

		float GetYaw() const { return m_Yaw; }
		float GetPitch() const { return m_Pitch; }
		void SetYawPitch(float yaw, float pitch) { m_Yaw = yaw; m_Pitch = pitch; RecalculateView(); }

		const glm::vec3& GetFront() const { return m_Front; }
		const glm::vec3& GetUp() const { return m_Up; }
		const glm::vec3& GetRight() const { return m_Right; }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	private:
		void RecalculateView();
	private:
		glm::mat4 m_ProjectionMatrix{ 1.0f };
		glm::mat4 m_ViewMatrix{ 1.0f };
		glm::mat4 m_ViewProjectionMatrix{ 1.0f };

		glm::vec3 m_Position = { 0.0f, 0.0f, 3.0f };
		glm::vec3 m_Front = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Right = { 1.0f, 0.0f, 0.0f };
		glm::vec3 m_WorldUp = { 0.0f, 1.0f, 0.0f };

		float m_Yaw = -90.0f;
		float m_Pitch = 0.0f;
	};
}
