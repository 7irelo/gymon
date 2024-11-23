workspace "Gymon"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

includeDir = {}
includeDir["GLFW"] = "Gymon/vendor/GLFW/include"

include "Gymon/vendor/GLFW"

project "Gymon"
	location "Gymon"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	pchheader "gypch.h"
	pchsource "Gymon/src/gypch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{includeDir.GLFW}"
	}

	links

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"GY_PLATFORM_WINDOWS",
			"GY_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines "GY_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "GY_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "GY_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("obj/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Gymon/vendor/spdlog/include",
		"Gymon/src"
	}

	links
	{
		"Gymon"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"GY_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "GY_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "GY_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "GY_DIST"
		optimize "On"