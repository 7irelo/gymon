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
