// LightYearsVisualGallery: interactive viewer and headless capture for the LightYearsVisual library.
//
//   LightYearsVisualGallery [--assets <dir>] [--script <name>] [--galaxy <progress>] [--backdrop-only] [--warp <beta>]
//   LightYearsVisualGallery --capture <out.png> [--script <name>] [--frames N] [--size WxH] [--stress] [--galaxy <progress>]
//                           [--backdrop-only] [--warp <beta>] [--assets <dir>]
//
// --galaxy 0..1 sets the place on the route from the Sun (0) to Sgr A* (1); --backdrop-only draws only the sky.
// --warp <beta> sets the ship speed (fraction of c, clamped to warp.maxBeta) for the star-field aberration.
// Keys: Tab color script, B bloom, D dither, V virtual resolution, N nebula divisor (1/2/4/8), Space energy-budget
// stress, Up/Down bloom strength, F backdrop only, W warp toggle (0 or 0.9 upward), PageUp/PageDown galaxy progress
// +-0.05, Home/End +-0.005, F8 reload assets (files are also polled), Esc quit.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <SFML/Graphics.hpp>

#include "GalleryScene.h"

using namespace ly::visual;

namespace
{
	struct Options
	{
		std::filesystem::path assets = LY_VISUAL_ASSETS_DIR;
		std::string capturePath;
		std::string script;
		int frames = 120;
		bool stress = false;
		bool backdropOnly = false;
		float galaxy = 0.f;
		float warp = 0.f;
		sf::Vector2u size{1280u, 720u};
	};

	bool ParseOptions(int argc, char** argv, Options& options)
	{
		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			bool hasValue = i + 1 < argc;
			if (arg == "--capture" && hasValue) options.capturePath = argv[++i];
			else if (arg == "--script" && hasValue) options.script = argv[++i];
			else if (arg == "--assets" && hasValue) options.assets = argv[++i];
			else if (arg == "--stress") options.stress = true;
			else if (arg == "--backdrop-only") options.backdropOnly = true;
			else if (arg == "--galaxy" && hasValue) options.galaxy = static_cast<float>(std::atof(argv[++i]));
			else if (arg == "--warp" && hasValue) options.warp = static_cast<float>(std::atof(argv[++i]));
			else if (arg == "--frames" && hasValue) options.frames = std::max(1, std::atoi(argv[++i]));
			else if (arg == "--size" && hasValue)
			{
				unsigned w = 0;
				unsigned h = 0;
				if (std::sscanf(argv[++i], "%ux%u", &w, &h) != 2 || w < 64 || h < 64)
				{
					std::cerr << "--size expects WxH, e.g. 1280x720\n";
					return false;
				}
				options.size = {w, h};
			}
			else
			{
				std::cerr << "unknown or incomplete argument: " << arg << "\n";
				return false;
			}
		}
		return true;
	}

	void PrintReport(const ReloadReport& report)
	{
		for (const std::string& line : report.reloaded)
		{
			std::cout << "reloaded " << line << std::endl;
		}
		for (const std::string& line : report.errors)
		{
			std::cerr << "RELOAD ERROR (previous version kept): " << line << "\n";
		}
	}

	// Digits with a comma every three places, e.g. 17300 -> 17,300.
	std::string WithThousands(long long value)
	{
		const std::string digits = std::to_string(value);
		std::string out;
		for (std::size_t i = 0; i < digits.size(); ++i)
		{
			if (i > 0 && (digits.size() - i) % 3 == 0)
			{
				out += ',';
			}
			out += digits[i];
		}
		return out;
	}

	// Shared by the window title and the capture log.
	std::string GalaxyText(const GalleryScene& scene)
	{
		const float progress = scene.GalaxyProgress();
		const galaxy::GalacticPosition position = galaxy::RoutePosition(progress);
		char text[64];
		std::snprintf(text, sizeof text, "%.3f", static_cast<double>(progress));
		const std::string progressText = text;
		std::snprintf(text, sizeof text, "%.*f", position.radiusKpc < 0.1f ? 3 : 1, static_cast<double>(position.radiusKpc));
		const std::string radiusText = text;
		const long long distanceLy = std::llround(galaxy::DistanceToCenterLy(position));
		std::snprintf(text, sizeof text, "%.2f", static_cast<double>(scene.WarpTarget()));
		const std::string warpText = text;
		return "galaxy p=" + progressText + " R=" + radiusText + " kpc " + WithThousands(distanceLy) + " ly | "
			+ galaxy::ArmName(scene.Sky().nearestArm) + " | flags " + scene.FeatureFlags() + " | warp " + warpText;
	}

	int Capture(const Options& options, VisualAssets& assets)
	{
		VisualPipeline pipeline(assets);
		if (!pipeline.Resize(options.size))
		{
			return 1;
		}
		GalleryScene scene(assets, pipeline);
		if (options.stress)
		{
			scene.ToggleStress();
		}
		scene.SetGalaxyProgress(options.galaxy);
		scene.SetBackdropOnly(options.backdropOnly);
		scene.SetWarp(options.warp, sf::Vector2f(0.f, -1.f));
		sf::RenderTexture target;
		if (!target.resize(options.size))
		{
			std::cerr << "cannot create the capture target\n";
			return 1;
		}
		for (int frame = 0; frame < options.frames; ++frame)
		{
			scene.Update(1.f / 60.f);
			pipeline.BeginFrame();
			scene.Draw();
			pipeline.Compose(target);
		}
		target.display();
		std::filesystem::path out = options.capturePath;
		if (out.has_parent_path())
		{
			std::filesystem::create_directories(out.parent_path());
		}
		if (!target.getTexture().copyToImage().saveToFile(out))
		{
			std::cerr << "cannot write " << out << "\n";
			return 1;
		}
		const EnergyStats& stats = pipeline.Stats();
		std::cout << "captured " << out.string() << " script=" << assets.Script().Name() << " frames=" << options.frames
			<< " energy " << stats.drawn << "/" << stats.submitted << " culled=" << stats.culledByBudget << "\n";
		std::cout << GalaxyText(scene) << "\n";
		return 0;
	}

	void UpdateTitle(sf::RenderWindow& window, VisualAssets& assets, const VisualPipeline& pipeline, const GalleryScene& scene, float fps)
	{
		const PipelineSettings& s = assets.Settings();
		const EnergyStats& e = pipeline.Stats();
		window.setTitle("LightYears Visual Gallery | " + assets.Script().Name() + " | world " + std::to_string(pipeline.VirtualSize().x) + "x"
			+ std::to_string(pipeline.VirtualSize().y) + " | bloom " + (s.bloomEnabled ? "on" : "off") + " | dither " + (s.ditherEnabled ? "on" : "off")
			+ " | nebula 1/" + std::to_string(s.nebulaResolutionDivisor)
			+ " | energy " + std::to_string(e.drawn) + "/" + std::to_string(e.submitted) + " culled " + std::to_string(e.culledByBudget)
			+ " | " + std::to_string(static_cast<int>(fps)) + " fps | " + GalaxyText(scene));
	}

	int RunWindow(const Options& options, VisualAssets& assets)
	{
		sf::RenderWindow window(sf::VideoMode(options.size), "LightYears Visual Gallery", sf::Style::Titlebar | sf::Style::Close);
		window.setFramerateLimit(60);
		VisualPipeline pipeline(assets);
		if (!pipeline.Resize(window.getSize()))
		{
			return 1;
		}
		GalleryScene scene(assets, pipeline);
		scene.SetGalaxyProgress(options.galaxy);
		scene.SetBackdropOnly(options.backdropOnly);
		scene.SetWarp(options.warp, sf::Vector2f(0.f, -1.f));
		sf::Clock clock;
		float titleTimer = 0.f;
		const int virtualHeights[] = {120, 180, 240, 360};
		const int nebulaDivisors[] = {1, 2, 4, 8};

		while (window.isOpen())
		{
			while (const std::optional event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					window.close();
				}
				else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
				{
					PipelineSettings& s = assets.Settings();
					switch (key->code)
					{
					case sf::Keyboard::Key::Escape: window.close(); break;
					case sf::Keyboard::Key::Tab: assets.Scripts().CycleActive(1); break;
					case sf::Keyboard::Key::B: s.bloomEnabled = !s.bloomEnabled; break;
					case sf::Keyboard::Key::D: s.ditherEnabled = !s.ditherEnabled; break;
					case sf::Keyboard::Key::Space: scene.ToggleStress(); break;
					case sf::Keyboard::Key::F: scene.SetBackdropOnly(!scene.BackdropOnly()); break;
					case sf::Keyboard::Key::W: scene.SetWarp(scene.WarpTarget() > 0.f ? 0.f : 0.9f, sf::Vector2f(0.f, -1.f)); break;
					case sf::Keyboard::Key::PageUp: scene.SetGalaxyProgress(scene.GalaxyProgress() + 0.05f); break;
					case sf::Keyboard::Key::PageDown: scene.SetGalaxyProgress(scene.GalaxyProgress() - 0.05f); break;
					case sf::Keyboard::Key::Home: scene.SetGalaxyProgress(scene.GalaxyProgress() + 0.005f); break;
					case sf::Keyboard::Key::End: scene.SetGalaxyProgress(scene.GalaxyProgress() - 0.005f); break;
					case sf::Keyboard::Key::Up: s.bloomStrength = std::min(s.bloomStrength + 0.1f, 4.f); break;
					case sf::Keyboard::Key::Down: s.bloomStrength = std::max(s.bloomStrength - 0.1f, 0.f); break;
					case sf::Keyboard::Key::V:
					{
						int next = virtualHeights[0];
						for (int i = 0; i < 4; ++i)
						{
							if (virtualHeights[i] == s.virtualHeight)
							{
								next = virtualHeights[(i + 1) % 4];
							}
						}
						s.virtualHeight = next;
						break;
					}
					case sf::Keyboard::Key::N:
					{
						int next = nebulaDivisors[0];
						for (int i = 0; i < 4; ++i)
						{
							if (nebulaDivisors[i] == s.nebulaResolutionDivisor)
							{
								next = nebulaDivisors[(i + 1) % 4];
							}
						}
						s.nebulaResolutionDivisor = next;
						break;
					}
					case sf::Keyboard::Key::F8: PrintReport(assets.Reload(true)); break;
					default: break;
					}
				}
			}

			float dt = clock.restart().asSeconds();
			PrintReport(assets.Poll(dt));
			scene.Update(dt);
			pipeline.BeginFrame();
			scene.Draw();
			window.clear();
			pipeline.Compose(window);
			window.display();

			titleTimer += dt;
			if (titleTimer > 0.4f)
			{
				titleTimer = 0.f;
				UpdateTitle(window, assets, pipeline, scene, dt > 0.f ? 1.f / dt : 0.f);
			}
		}
		return 0;
	}
}

int main(int argc, char** argv)
{
	Options options;
	if (!ParseOptions(argc, argv, options))
	{
		return 2;
	}
	if (!sf::Shader::isAvailable())
	{
		std::cerr << "shaders are not available on this system\n";
		return 1;
	}
	VisualAssets assets;
	std::string error;
	if (!assets.Load(options.assets, error))
	{
		std::cerr << "cannot load assets from " << options.assets.string() << ": " << error << "\n";
		return 1;
	}
	PrintReport(assets.LoadReport());
	if (!options.script.empty() && !assets.Scripts().SetActive(options.script))
	{
		std::cerr << "unknown color script \"" << options.script << "\"\n";
		return 2;
	}
	return options.capturePath.empty() ? RunWindow(options, assets) : Capture(options, assets);
}
