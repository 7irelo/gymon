#pragma once

#include <memory>
#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Gymon {

	class GYMON_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}


// Core log macros
#define GY_CORE_TRACE(...)  ::Gymon::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GY_CORE_INFO(...)   ::Gymon::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GY_CORE_WARN(...)   ::Gymon::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GY_CORE_ERROR(...)  ::Gymon::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GY_CORE_FATAL(...)  ::Gymon::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define GY_TRACE(...)		::Gymon::Log::GetClientLogger()->trace(__VA_ARGS__)
#define GY_INFO(...)		::Gymon::Log::GetClientLogger()->info(__VA_ARGS__)
#define GY_WARN(...)		::Gymon::Log::GetClientLogger()->warn(__VA_ARGS__)
#define GY_ERROR(...)		::Gymon::Log::GetClientLogger()->error(__VA_ARGS__)
#define GY_FATAL(...)		::Gymon::Log::GetClientLogger()->fatal(__VA_ARGS__)