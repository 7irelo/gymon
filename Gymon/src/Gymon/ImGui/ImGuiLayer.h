#pragma once

#include "Gymon/Layer.h"

struct ImFont;

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

		// Loads a UI and a monospaced face, preferring a font bundled with the
		// application and falling back to a platform font, then to ImGui's
		// built-in bitmap face.
		void LoadFonts();

		// Rounding, padding and spacing tuned for an editor rather than for a
		// debug overlay.
		void SetStyle();

		// Monospaced face for columnar readouts. Null when none was found.
		ImFont* GetMonoFont() const { return m_MonoFont; }
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
		ImFont* m_MonoFont = nullptr;
	};
}
