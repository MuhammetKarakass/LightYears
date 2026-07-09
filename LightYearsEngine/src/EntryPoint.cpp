#include "EntryPoint.h"
#include "framework/Application.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

int main() {
	try
	{
		std::unique_ptr<ly::Application> app{ GetApplication() };
		if (!app)
		{
			std::cerr << "Failed to create application." << std::endl;
			return 1;
		}

		app->Run();
		app.reset();
		std::quick_exit(0);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Unhandled exception: " << exception.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unhandled unknown exception." << std::endl;
		return 1;
	}

	return 0;
}
