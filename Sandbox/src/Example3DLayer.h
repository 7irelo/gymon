#pragma once

#include <Gymon.h>

class Example3DLayer : public Gymon::Layer
{
public:
	Example3DLayer();
	virtual ~Example3DLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Gymon::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Gymon::Event& e) override;

	void SetActive(bool active) { m_Active = active; }
	bool IsActive() const { return m_Active; }
private:
	bool m_Active = false;
	Gymon::PerspectiveCameraController m_CameraController;

	Gymon::Ref<Gymon::Mesh> m_CubeMesh;
	Gymon::Ref<Gymon::Shader> m_LightingShader;

	glm::vec3 m_LightPosition = { 2.0f, 4.0f, 2.0f };
	glm::vec3 m_LightColor = { 1.0f, 1.0f, 1.0f };
	glm::vec3 m_ObjectColor = { 0.8f, 0.4f, 0.3f };

	float m_Angle = 0.0f;
};
