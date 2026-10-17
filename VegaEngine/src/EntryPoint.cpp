#include "EntryPoint.h"
#include "framework/Application.h"
#include "framework/AudioOutput.h"
#include "framework/ProjectSettings.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"
#include <cstdlib>
#include <exception>
#include <memory>

int main(int argc, char** argv)
{
	vega::AudioOutput::Initialize(argc, argv);
	vega::debug::Profiler::Initialize();
	try
	{
		std::string logName = "Vega.log";
		// The host's project owns log naming; the engine has no concrete game name.
#ifdef _MSC_VER
		auto project = std::filesystem::absolute(__argv[0]).parent_path() / "assets/vega.project.json";
		for (int i = 1; i + 1 < __argc; ++i)
		{
			const std::string argument{ __argv[i] };
			if (argument == "--project") project = __argv[++i];
			else if (argument == "--asset-root")
			{
				const auto candidate = std::filesystem::path{ __argv[++i] } / "vega.project.json";
				if (std::filesystem::exists(candidate)) project = candidate;
			}
		}
		if (std::filesystem::exists(project))
			logName = vega::ProjectSettings::Load(project).runtimeLog;
#endif
		vega::debug::Log::Initialize(logName);
		std::unique_ptr<vega::Application> app{ GetApplication() };
		if (!app)
		{
			VEGA_CORE_FATAL("Failed to create application");
			vega::debug::Profiler::Shutdown();
			vega::debug::Log::Shutdown();
			return 1;
		}

		app->Run();
		const int exitCode = app->GetExitCode();
		app.reset();
		vega::debug::Profiler::Shutdown();
		vega::debug::Log::Shutdown();
		std::quick_exit(exitCode);
	}
	catch (const std::exception& exception)
	{
		VEGA_CORE_FATAL("Unhandled exception: %s", exception.what());
		vega::debug::Profiler::Shutdown();
		vega::debug::Log::Shutdown();
		return 1;
	}
	catch (...)
	{
		VEGA_CORE_FATAL("Unhandled unknown exception");
		vega::debug::Profiler::Shutdown();
		vega::debug::Log::Shutdown();
		return 1;
	}
}
