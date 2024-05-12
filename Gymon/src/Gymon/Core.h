#pragma once

#ifdef GY_PLATFORM_WINDOWS
	#ifdef GY_BUILD_DLL
		#define GYMON_API __declspec(dllexport)
	#else
		#define GYMON_API __declspec(dllimport)
	#endif
#else 
	#error Gymon only supports Windows!
#endif

