#include <Gymon.h>
#include <Gymon/EntryPoint.h>

#include "Sandbox2D.h"
#include "Example3DLayer.h"
#include "EditorLayer.h"

#include <imgui.h>

// Overlay that draws a menu bar to switch between the 2D and 3D demo scenes.
class SceneSwitcherLayer : public Gymon::Layer
{
public:
	SceneSwitcherLayer(Sandbox2D* layer2D, Example3DLayer* layer3D, EditorLayer* editor)
		: Layer("SceneSwitcher"), m_Layer2D(layer2D), m_Layer3D(layer3D), m_Editor(editor)
	{
	}

	virtual void OnImGuiRender() override
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("2D Renderer", nullptr, m_Layer2D->IsActive()))
				{
					m_Layer2D->SetActive(true);
					m_Layer3D->SetActive(false);
					m_Editor->SetActive(false);
				}
				if (ImGui::MenuItem("3D Renderer", nullptr, m_Layer3D->IsActive()))
				{
					m_Layer2D->SetActive(false);
					m_Layer3D->SetActive(true);
					m_Editor->SetActive(false);
				}
				if (ImGui::MenuItem("Editor", nullptr, m_Editor->IsActive()))
				{
					m_Layer2D->SetActive(false);
					m_Layer3D->SetActive(false);
					m_Editor->SetActive(true);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					Gymon::Application::Get().Close();

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}
private:
	Sandbox2D* m_Layer2D;
	Example3DLayer* m_Layer3D;
	EditorLayer* m_Editor;
};

class Sandbox : public Gymon::Application
{
public:
	Sandbox()
		: Application("Gymon Engine - Sandbox")
	{
		Sandbox2D* layer2D = new Sandbox2D();
		Example3DLayer* layer3D = new Example3DLayer();
		EditorLayer* editor = new EditorLayer();

		PushLayer(layer2D);
		PushLayer(layer3D);
		PushLayer(editor);
		PushOverlay(new SceneSwitcherLayer(layer2D, layer3D, editor));
	}

	~Sandbox()
	{
	}
};

Gymon::Application* Gymon::CreateApplication()
{
	return new Sandbox();
}
