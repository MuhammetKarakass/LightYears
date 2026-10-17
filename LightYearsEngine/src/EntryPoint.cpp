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
	ly::AudioOutput::Initialize(argc, argv);
	ly::debug::Profiler::Initialize();
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
			logName = ly::ProjectSettings::Load(project).runtimeLog;
#endif
		ly::debug::Log::Initialize(logName);
		std::unique_ptr<ly::Application> app{ GetApplication() };
		if (!app)
		{
			LY_CORE_FATAL("Failed to create application");
			ly::debug::Profiler::Shutdown();
			ly::debug::Log::Shutdown();
			return 1;
		}

		app->Run();
		const int exitCode = app->GetExitCode();
		app.reset();
		ly::debug::Profiler::Shutdown();
		ly::debug::Log::Shutdown();
		std::quick_exit(exitCode);
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
}
