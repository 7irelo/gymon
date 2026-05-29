#include "gypch.h"
#include "Gymon/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Gymon {

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
		RecalculateView();
	}

	void PerspectiveCamera::SetProjection(float fov, float aspectRatio, float nearClip, float farClip)
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::RecalculateView()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		m_Front = glm::normalize(front);

		m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}
