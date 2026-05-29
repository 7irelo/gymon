#include "gypch.h"
#include "Gymon/PerspectiveCameraController.h"

#include "Gymon/Input.h"
#include "Gymon/KeyCodes.h"
#include "Gymon/MouseCodes.h"

#include <algorithm>

namespace Gymon {

	PerspectiveCameraController::PerspectiveCameraController(float aspectRatio, float fov)
		: m_AspectRatio(aspectRatio), m_Fov(fov), m_Camera(fov, aspectRatio, 0.1f, 1000.0f)
	{
		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetYawPitch(m_Yaw, m_Pitch);
	}

	void PerspectiveCameraController::OnUpdate(Timestep ts)
	{
		float velocity = m_MoveSpeed * ts;
		const glm::vec3& front = m_Camera.GetFront();
		const glm::vec3& right = m_Camera.GetRight();
		const glm::vec3& up = m_Camera.GetUp();

		if (Input::IsKeyPressed(Key::W))
			m_CameraPosition += front * velocity;
		if (Input::IsKeyPressed(Key::S))
			m_CameraPosition -= front * velocity;
		if (Input::IsKeyPressed(Key::A))
			m_CameraPosition -= right * velocity;
		if (Input::IsKeyPressed(Key::D))
			m_CameraPosition += right * velocity;
		if (Input::IsKeyPressed(Key::Space))
			m_CameraPosition += up * velocity;
		if (Input::IsKeyPressed(Key::LeftShift))
			m_CameraPosition -= up * velocity;

		// Mouse look while holding the right mouse button.
		if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
		{
			glm::vec2 mouse = Input::GetMousePosition();
			if (m_FirstMouse)
			{
				m_LastMousePos = mouse;
				m_FirstMouse = false;
			}

			float xOffset = (mouse.x - m_LastMousePos.x) * m_MouseSensitivity;
			float yOffset = (m_LastMousePos.y - mouse.y) * m_MouseSensitivity;
			m_LastMousePos = mouse;

			m_Yaw += xOffset;
			m_Pitch += yOffset;
			m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

			m_Camera.SetYawPitch(m_Yaw, m_Pitch);
		}
		else
		{
			m_FirstMouse = true;
		}

		m_Camera.SetPosition(m_CameraPosition);
	}

	void PerspectiveCameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(GY_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(GY_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
	}

	void PerspectiveCameraController::OnResize(float width, float height)
	{
		m_AspectRatio = width / height;
		m_Camera.SetProjection(m_Fov, m_AspectRatio, 0.1f, 1000.0f);
	}

	bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		m_Fov -= e.GetYOffset() * 2.0f;
		m_Fov = std::clamp(m_Fov, 1.0f, 90.0f);
		m_Camera.SetProjection(m_Fov, m_AspectRatio, 0.1f, 1000.0f);
		return false;
	}

	bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		OnResize((float)e.GetWidth(), (float)e.GetHeight());
		return false;
	}
}
