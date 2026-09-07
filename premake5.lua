workspace "Gymon"
	architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to the root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Gymon/vendor/GLFW/include"
IncludeDir["Glad"] = "Gymon/vendor/Glad/include"
IncludeDir["ImGui"] = "Gymon/vendor/imgui"
IncludeDir["glm"] = "Gymon/vendor/glm"
IncludeDir["stb_image"] = "Gymon/vendor/stb_image"
IncludeDir["json"] = "Gymon/vendor/json"

group "Dependencies"
	include "Gymon/vendor/GLFW"
	include "Gymon/vendor/Glad"

	project "ImGui"
		location "Gymon/vendor/imgui"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "off"

		targetdir ("bin/" .. outputdir .. "/%{prj.name}")
		objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

		files
		{
			"Gymon/vendor/imgui/imconfig.h",
			"Gymon/vendor/imgui/imgui.h",
			"Gymon/vendor/imgui/imgui.cpp",
			"Gymon/vendor/imgui/imgui_draw.cpp",
			"Gymon/vendor/imgui/imgui_internal.h",
			"Gymon/vendor/imgui/imgui_tables.cpp",
			"Gymon/vendor/imgui/imgui_widgets.cpp",
			"Gymon/vendor/imgui/imstb_rectpack.h",
			"Gymon/vendor/imgui/imstb_textedit.h",
			"Gymon/vendor/imgui/imstb_truetype.h",
			"Gymon/vendor/imgui/imgui_demo.cpp"
		}

		filter "system:windows"
			systemversion "latest"

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			runtime "Release"
			optimize "on"
			symbols "off"
group ""

project "Gymon"
	location "Gymon"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	pchheader "gypch.h"
	pchsource "Gymon/src/gypch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.cpp",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{prj.name}/vendor"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	-- Vendor sources don't use the engine's precompiled header.
	filter "files:Gymon/vendor/**.cpp"
		flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"GY_PLATFORM_WINDOWS",
			"GY_BUILD_DLL",
			"NOMINMAX"
		}

	filter "configurations:Debug"
		defines "GY_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GY_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "GY_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	debugdir "%{prj.location}"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Gymon/vendor/spdlog/include",
		"Gymon/src",
		"Gymon/vendor",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Gymon"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"GY_PLATFORM_WINDOWS",
			"NOMINMAX"
		}

	filter "configurations:Debug"
		defines "GY_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GY_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "GY_DIST"
		runtime "Release"
		optimize "on"
