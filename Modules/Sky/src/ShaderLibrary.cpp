#include "sky/ShaderLibrary.h"

#include <algorithm>
#include <sstream>

#include <SFML/System/Err.hpp>

#include "FileUtil.h"
#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		// SFML reports compile errors only through sf::err(); capture them so a failed reload can be shown.
		bool CompileFragment(const std::filesystem::path& file, sf::Shader& shader, std::string& error)
		{
			std::string source;
			if (!ReadTextFile(file, source))
			{
				error = "cannot read " + file.u8string();
				return false;
			}
			std::ostringstream captured;
			std::streambuf* previous = sf::err().rdbuf(captured.rdbuf());
			bool ok = shader.loadFromMemory(source, sf::Shader::Type::Fragment);
			sf::err().rdbuf(previous);
			if (!ok)
			{
				error = captured.str();
			}
			return ok;
		}
	}

	bool ShaderLibrary::LoadDirectory(const std::filesystem::path& dir, std::string& error)
	{
		if (!sf::Shader::isAvailable())
		{
			error = "shaders are not available on this system";
			return false;
		}
		mDirectory = dir;
		mShaders.clear();
		mSeenStamps.clear();
		ReloadReport report;
		Reload(true, report);
		if (mShaders.empty() || !report.errors.empty())
		{
			error = "shader load from " + dir.u8string() + " failed";
			for (const std::string& e : report.errors)
			{
				error += "\n  " + e;
			}
			return false;
		}
		return true;
	}

	void ShaderLibrary::Reload(bool force, ReloadReport& report)
	{
		std::vector<std::filesystem::path> files;
		std::error_code ec;
		for (const auto& item : std::filesystem::directory_iterator(mDirectory, ec))
		{
			if (item.is_regular_file() && item.path().extension() == ".frag")
			{
				files.push_back(item.path());
			}
		}
		std::sort(files.begin(), files.end());

		for (const auto& file : files)
		{
			auto stamp = FileStamp(file);
			auto seen = mSeenStamps.find(file);
			if (!force && seen != mSeenStamps.end() && seen->second == stamp)
			{
				continue;
			}
			mSeenStamps[file] = stamp;
			auto shader = std::make_unique<sf::Shader>();
			std::string error;
			if (!CompileFragment(file, *shader, error))
			{
				report.errors.push_back("shader " + file.filename().u8string() + ": " + error);
				continue;
			}
			mShaders[file.stem().u8string()] = {file, std::move(shader)};
			UniformWriter::InvalidateLocations();
			report.reloaded.push_back("shader " + file.filename().u8string());
		}
	}

	sf::Shader* ShaderLibrary::Find(const std::string& name)
	{
		auto it = mShaders.find(name);
		return it == mShaders.end() ? nullptr : it->second.shader.get();
	}

	void SetRampUniform(sf::Shader& shader, const std::string& name, const ColorRamp& ramp, int count)
	{
		std::vector<sf::Glsl::Vec3> colors;
		for (int i = 0; i < count; ++i)
		{
			sf::Color c = ramp.Sample(count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.f);
			colors.emplace_back(c.r / 255.f, c.g / 255.f, c.b / 255.f);
		}
		UniformWriter(shader).SetArray(name.c_str(), colors.data(), colors.size());
	}

	void DrawShaderQuad(sf::RenderTarget& target, sf::Shader& shader, const sf::FloatRect& rect, const sf::BlendMode& blend)
	{
		float l = rect.position.x;
		float t = rect.position.y;
		float r = l + rect.size.x;
		float b = t + rect.size.y;
		sf::Vertex quad[4] = {
			{{l, t}, sf::Color::White, {l, t}},
			{{r, t}, sf::Color::White, {r, t}},
			{{l, b}, sf::Color::White, {l, b}},
			{{r, b}, sf::Color::White, {r, b}},
		};
		sf::RenderStates states;
		states.blendMode = blend;
		states.shader = &shader;
		target.draw(quad, 4, sf::PrimitiveType::TriangleStrip, states);
	}
}
