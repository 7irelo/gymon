#pragma once

#include "Gymon/Core.h"
#include "Gymon/KeyCodes.h"
#include "Gymon/MouseCodes.h"

#include <glm/glm.hpp>

namespace Gymon {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);

		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
