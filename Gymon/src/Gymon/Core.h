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

#ifdef GY_ENABLE_ASSERTS
	#define GY_ASSERT(x, ...) { if(!(x)) { GY_ERROR("Assersion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define GY_CORE_ASSERT(X, ...) { if(!(x)) {GY_CORE_ERROR("Assersion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define GY_ASSERT(x, ...)
	#define GY_CORE_ASSERT(X, ...)
#endif

#define BIT(x) (1 << x)