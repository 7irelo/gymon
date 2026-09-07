#pragma once

#include "Gymon/Renderer/PerspectiveCamera.h"
#include "Gymon/Core/Timestep.h"

#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Events/MouseEvent.h"

namespace Gymon {

	// First-person style controller: WASD to move, hold right mouse button to look around.
	class PerspectiveCameraController
	{
	public:
		PerspectiveCameraController(float aspectRatio, float fov = 45.0f);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		void OnResize(float width, float height);

		PerspectiveCamera& GetCamera() { return m_Camera; }
		const PerspectiveCamera& GetCamera() const { return m_Camera; }

		// The controller owns the camera's position and orientation and
		// rewrites them every update, so setting them on the camera directly
		// has no lasting effect. These are the ones that stick.
		void SetPosition(const glm::vec3& position);
		void SetYawPitch(float yaw, float pitch);

		const glm::vec3& GetPosition() const { return m_CameraPosition; }

		// Pulls the camera back far enough to fit a sphere of the given radius
		// in view, keeping the current viewing angle. The editor equivalent of
		// "frame selection".
		void Focus(const glm::vec3& target, float radius);
	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
	private:
		float m_AspectRatio;
		float m_Fov;
		PerspectiveCamera m_Camera;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 3.0f };
		float m_Yaw = -90.0f;
		float m_Pitch = 0.0f;

		float m_MoveSpeed = 5.0f;
		float m_MouseSensitivity = 0.1f;

		glm::vec2 m_LastMousePos = { 0.0f, 0.0f };
		bool m_FirstMouse = true;
	};
}
