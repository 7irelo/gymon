#pragma once

#include <memory>

#ifdef GY_PLATFORM_WINDOWS
#else
	#error Gymon only supports Windows!
#endif

// Gymon is built as a static library, so no import/export decoration is needed.
// The macro is kept for source compatibility with older headers.
#define GYMON_API

#ifdef GY_DEBUG
	#define GY_ENABLE_ASSERTS
#endif

#ifdef GY_ENABLE_ASSERTS
	#define GY_ASSERT(x, ...) { if(!(x)) { GY_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define GY_CORE_ASSERT(x, ...) { if(!(x)) { GY_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define GY_ASSERT(x, ...)
	#define GY_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define GY_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Gymon {

	// Scope == unique ownership, Ref == shared ownership.
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}
