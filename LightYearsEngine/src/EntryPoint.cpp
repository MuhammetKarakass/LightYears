#include "EntryPoint.h"
#include "framework/Application.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"
#include <cstdlib>
#include <exception>
#include <memory>

int main() {
	ly::debug::Log::Initialize("LightYears.log");
	ly::debug::Profiler::Initialize();
	try
	{
		std::unique_ptr<ly::Application> app{ GetApplication() };
		if (!app)
		{
			LY_CORE_FATAL("Failed to create application");
			ly::debug::Profiler::Shutdown();
			ly::debug::Log::Shutdown();
			return 1;
		}

		app->Run();
		app.reset();
		ly::debug::Profiler::Shutdown();
		ly::debug::Log::Shutdown();
		std::quick_exit(0);
	}
	catch (const std::exception& exception)
	{
		LY_CORE_FATAL("Unhandled exception: %s", exception.what());
		ly::debug::Profiler::Shutdown();
		ly::debug::Log::Shutdown();
		return 1;
	}
	catch (...)
	{
		LY_CORE_FATAL("Unhandled unknown exception");
		ly::debug::Profiler::Shutdown();
		ly::debug::Log::Shutdown();
		return 1;
	}

	return 0;
}
