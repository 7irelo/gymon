#pragma once

#include "Gymon/Layer.h"

#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Events/KeyEvent.h"
#include "Gymon/Events/MouseEvent.h"

namespace Gymon {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		// When block is enabled, ImGui consumes input events it handles.
		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetDarkThemeColors();
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
	};
}
